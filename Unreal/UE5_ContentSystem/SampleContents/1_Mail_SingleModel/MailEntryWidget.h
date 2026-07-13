#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"
#include "MailEntryWidget.generated.h"

class UCommonTextBlock;

// 메일 목록의 메일 표현을 위한 위젯 클래스
UCLASS(Abstract)
class PACONTENTSYSTEM_API UMailEntryWidget : public UCommonUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PA|Mail")
    TObjectPtr<UCommonTextBlock> TitleText;
};
