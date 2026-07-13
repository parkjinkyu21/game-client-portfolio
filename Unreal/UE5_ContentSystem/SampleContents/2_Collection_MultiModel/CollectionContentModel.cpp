#include "CollectionContentModel.h"
#include "CollectionAppearanceModel.h"
#include "CollectionTitleModel.h"
#include "CollectionContentEvents.h"
#include "Framework/PAContentModelManagerBase.h"

void UCollectionContentModel::BindPacketHandlers()
{
    // 로그인 시점에 컬렉션 요약 정보를 요청하도록 등록한다.
    BindLoginRequest<FCollectionSummaryRequest>(this, &UCollectionContentModel::OnSummaryResponse);
}

void UCollectionContentModel::OnInitialize()
{
    // 자식의 캐시 데이터가 변경됨을 구독한다.
    for (UPAContentModelBase* Child : GetChildren())
    {
        Child->OnDataChanged.AddUObject(this, &UCollectionContentModel::RecalculateProgress);
    }
}

// 컬렉션 요약 정보 응답
void UCollectionContentModel::OnSummaryResponse(EPAContentRequestResult Result, const FCollectionSummaryResponse& Response)
{
    if (Result != EPAContentRequestResult::Success)
    {
        return;
    }

    AppearanceCollected = Response.AppearanceCollected;
    AppearanceTotal = Response.AppearanceTotal;
    TitleOwned = Response.TitleOwned;
    TitleTotal = Response.TitleTotal;

    // 요약 정보가 변경되면 Collection_Event_Progress 이벤트를 발생한다.
    BroadcastProgress();
    OnDataChanged.Broadcast();
}

void UCollectionContentModel::RecalculateProgress()
{
    // 자식 모델의 캐시가 변경되면 컬렉션 모델의 요약 정보를 갱신하자.
    for (const UPAContentModelBase* Child : GetChildren())
    {
        if (const UCollectionAppearanceModel* Appearance = Cast<UCollectionAppearanceModel>(Child))
        {
            const TArray<FAppearanceEntry>& Entries = Appearance->GetAppearances();
            if (Entries.Num() > 0)
            {
                AppearanceTotal = Entries.Num();
                AppearanceCollected = 0;
                for (const FAppearanceEntry& Entry : Entries)
                {
                    AppearanceCollected += Entry.bCollected ? 1 : 0;
                }
            }
        }
        else if (const UCollectionTitleModel* Title = Cast<UCollectionTitleModel>(Child))
        {
            const TArray<FTitleEntry>& Entries = Title->GetTitles();
            if (Entries.Num() > 0)
            {
                TitleTotal = Entries.Num();
                TitleOwned = 0;
                for (const FTitleEntry& Entry : Entries)
                {
                    TitleOwned += Entry.bOwned ? 1 : 0;
                }
            }
        }
    }

    BroadcastProgress();
    OnDataChanged.Broadcast();
}

void UCollectionContentModel::OnReset()
{
    AppearanceCollected = 0;
    AppearanceTotal = 0;
    TitleOwned = 0;
    TitleTotal = 0;
}

void UCollectionContentModel::BroadcastProgress()
{
    UPAContentModelManagerBase::BroadcastContentEvent(this, PAContentEvents::Collection_Event_Progress,
        FCollectionProgressChanged(AppearanceCollected, AppearanceTotal, TitleOwned, TitleTotal));
}
