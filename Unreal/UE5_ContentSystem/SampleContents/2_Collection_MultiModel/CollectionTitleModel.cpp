#include "CollectionTitleModel.h"
#include "CollectionContentModel.h"

UCollectionTitleModel::UCollectionTitleModel()
{
    ParentModelClass = UCollectionContentModel::StaticClass();

    // 목록은 한번만 요청하여 캐싱하고 , 이후 동기화 패킷에 의하여 캐시 정보를 수정하여 사용한다.
    CachePolicy = EPAContentCachePolicy::CacheUntilInvalidated;
}

void UCollectionTitleModel::BindPacketHandlers()
{
    BindCachedDataRequest<FTitleListRequest>(this, &UCollectionTitleModel::OnListResponse);
    BindActionRequest<FTitleEquipRequest>(this, &UCollectionTitleModel::OnEquipResponse);
    BindNotify<FTitleAcquiredNotify>(this, &UCollectionTitleModel::OnTitleAcquiredNotify);
}

void UCollectionTitleModel::RequestEquip(FName TitleId)
{
    FTitleEquipRequest Request;
    Request.TitleId = TitleId;
    SendActionRequest(MoveTemp(Request));
}

// 캐시 데이터 요청 응답: 타이틀 목록
void UCollectionTitleModel::OnListResponse(EPAContentRequestResult Result, const FTitleListResponse& Response)
{
    if (Result != EPAContentRequestResult::Success)
    {
        return;
    }

    Titles = Response.Titles;
    OnDataChanged.Broadcast();
}

// 타이틀 장착 요청 응답
void UCollectionTitleModel::OnEquipResponse(EPAContentRequestResult Result, const FTitleEquipResponse& Response)
{
    if (Result != EPAContentRequestResult::Success || !Response.bSuccess)
    {
        return;
    }

    // 모델 데이터에 반영
    for (FTitleEntry& Entry : Titles)
    {
        // 타이틀은 1개만 장착 가능하다 가정하자.
        Entry.bEquipped = (Entry.TitleId == Response.TitleId);
    }
    OnDataChanged.Broadcast();
}

// 타이틀을 획득 했다는 노티
void UCollectionTitleModel::OnTitleAcquiredNotify(const FTitleAcquiredNotify& Notify)
{
    for (FTitleEntry& Entry : Titles)
    {
        // 모델 데이터에 반영
        if (Entry.TitleId == Notify.TitleId)
        {
            Entry.bOwned = true;
            OnDataChanged.Broadcast();
            return;
        }
    }
}

void UCollectionTitleModel::OnReset()
{
    Titles.Reset();
}
