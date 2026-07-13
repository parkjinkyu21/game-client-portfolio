#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentModelBase.h"
#include "CollectionContentMessages.h"
#include "CollectionAppearanceModel.generated.h"

// 외형 도감 모델.
UCLASS()
class PACONTENTSYSTEM_API UCollectionAppearanceModel : public UPAContentModelBase
{
    GENERATED_BODY()

public:
    UCollectionAppearanceModel();

    const TArray<FAppearanceEntry>& GetAppearances() const { return Appearances; }
    void RequestRegister(FName AppearanceId);

protected:
    virtual void BindPacketHandlers() override;
    virtual void OnReset() override;

private:
    void OnListResponse(EPAContentRequestResult Result, const FAppearanceListResponse& Response);
    void OnRegisterResponse(EPAContentRequestResult Result, const FAppearanceRegisterResponse& Response);
    void OnAppearanceAcquiredNotify(const FAppearanceAcquiredNotify& Notify);

    UPROPERTY()
    TArray<FAppearanceEntry> Appearances;
};
