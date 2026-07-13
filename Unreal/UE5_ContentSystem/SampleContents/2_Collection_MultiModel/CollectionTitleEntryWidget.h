#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"
#include "CollectionTitleEntryWidget.generated.h"

class UCommonTextBlock;

// 타이틀 목록의 타이틀 표현을 위한 위젯 클래스
UCLASS(Abstract)
class PACONTENTSYSTEM_API UCollectionTitleEntryWidget : public UCommonUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> NameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> GradeText;

    // 장착 중 표시 — 미장착이면 숨긴다
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> EquippedText;
};
