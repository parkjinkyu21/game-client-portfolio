#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentModelBase.h"
#include "CollectionContentMessages.h"
#include "CollectionTitleModel.generated.h"

// 타이틀 도감 모델.
UCLASS()
class PACONTENTSYSTEM_API UCollectionTitleModel : public UPAContentModelBase
{
    GENERATED_BODY()

public:
    UCollectionTitleModel();

    const TArray<FTitleEntry>& GetTitles() const { return Titles; }
    void RequestEquip(FName TitleId);

protected:
    virtual void BindPacketHandlers() override;
    virtual void OnReset() override;

private:
    void OnListResponse(EPAContentRequestResult Result, const FTitleListResponse& Response);
    void OnEquipResponse(EPAContentRequestResult Result, const FTitleEquipResponse& Response);
    void OnTitleAcquiredNotify(const FTitleAcquiredNotify& Notify);

    UPROPERTY()
    TArray<FTitleEntry> Titles;
};
