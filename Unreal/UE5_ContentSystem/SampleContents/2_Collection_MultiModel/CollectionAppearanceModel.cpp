#include "CollectionAppearanceModel.h"
#include "CollectionContentModel.h"

UCollectionAppearanceModel::UCollectionAppearanceModel()
{
    ParentModelClass = UCollectionContentModel::StaticClass();

    // 목록은 한번만 요청하여 캐싱하고 , 이후 동기화 패킷에 의하여 캐시 정보를 수정하여 사용한다.
    CachePolicy = EPAContentCachePolicy::CacheUntilInvalidated;
}

void UCollectionAppearanceModel::BindPacketHandlers()
{
    BindCachedDataRequest<FAppearanceListRequest>(this, &UCollectionAppearanceModel::OnListResponse);
    BindActionRequest<FAppearanceRegisterRequest>(this, &UCollectionAppearanceModel::OnRegisterResponse);
    BindNotify<FAppearanceAcquiredNotify>(this, &UCollectionAppearanceModel::OnAppearanceAcquiredNotify);
}

void UCollectionAppearanceModel::RequestRegister(FName AppearanceId)
{
    FAppearanceRegisterRequest Request;
    Request.AppearanceId = AppearanceId;
    SendActionRequest(MoveTemp(Request));
}

// 캐시 데이터 요청 응답: 외형 도감 목록
void UCollectionAppearanceModel::OnListResponse(EPAContentRequestResult Result, const FAppearanceListResponse& Response)
{
    if (Result != EPAContentRequestResult::Success)
    {
        return;
    }

    Appearances = Response.Appearances;
    OnDataChanged.Broadcast();
}

// 조작 요청 응답: 외형 등록 — 모델 데이터에 반영
void UCollectionAppearanceModel::OnRegisterResponse(EPAContentRequestResult Result, const FAppearanceRegisterResponse& Response)
{
    if (Result != EPAContentRequestResult::Success || !Response.bSuccess)
    {
        return;
    }

    for (FAppearanceEntry& Entry : Appearances)
    {
        if (Entry.AppearanceId == Response.AppearanceId)
        {
            Entry.bCollected = true;
            break;
        }
    }
    OnDataChanged.Broadcast();
}

// 외형 획득 노티: 캐시 정보 갱신
void UCollectionAppearanceModel::OnAppearanceAcquiredNotify(const FAppearanceAcquiredNotify& Notify)
{
    for (FAppearanceEntry& Entry : Appearances)
    {
        if (Entry.AppearanceId == Notify.AppearanceId)
        {
            Entry.bCollected = true;
            OnDataChanged.Broadcast();
            return;
        }
    }
}

void UCollectionAppearanceModel::OnReset()
{
    Appearances.Reset();
}
