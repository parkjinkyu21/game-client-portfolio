#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentModelBase.h"
#include "CollectionContentMessages.h"
#include "CollectionContentModel.generated.h"

// 컬렉션 컨텐츠 메인 모델.
UCLASS()
class PACONTENTSYSTEM_API UCollectionContentModel : public UPAContentModelBase
{
    GENERATED_BODY()

public:
    int32 GetAppearanceCollected() const { return AppearanceCollected; }
    int32 GetAppearanceTotal() const { return AppearanceTotal; }
    int32 GetTitleOwned() const { return TitleOwned; }
    int32 GetTitleTotal() const { return TitleTotal; }

protected:
    virtual void BindPacketHandlers() override;
    virtual void OnInitialize() override; // 자식 도감 모델들의 OnDataChanged 구독
    virtual void OnReset() override;

private:
    void OnSummaryResponse(EPAContentRequestResult Result, const FCollectionSummaryResponse& Response);

    // 자식 도감 데이터에서 진행 수치를 다시 계산하고 Collection_Event_Progress 이벤트를 발생시킨다
    void RecalculateProgress();
    void BroadcastProgress();

    int32 AppearanceCollected = 0;
    int32 AppearanceTotal = 0;
    int32 TitleOwned = 0;
    int32 TitleTotal = 0;
};
