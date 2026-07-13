#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"
#include "CollectionAppearanceEntryWidget.generated.h"

class UCommonTextBlock;

// 외형 도감 목록의 외형 표현을 위한 위젯 클래스
UCLASS(Abstract)
class PACONTENTSYSTEM_API UCollectionAppearanceEntryWidget : public UCommonUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> NameText;
};
