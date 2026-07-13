#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "NativeGameplayTags.h"
#include "MailContentMessages.h"
#include "MailInboxWidget.generated.h"

class UListView;

// PAContentUITag 네임스페이스는 모든 UI가 공유한다 — 각각의 UI 클래스에서 UI id 태그를 정의 하자.
namespace PAContentUITag
{
    PACONTENTSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Mail_Inbox);
}

// ListView 항목 오브젝트
UCLASS()
class PACONTENTSYSTEM_API UMailEntryObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "PA|Mail")
    FMailData Mail;
};

// 메일 목록 표시를 위한 위젯 클래스
UCLASS(Abstract)
class PACONTENTSYSTEM_API UMailInboxWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void HandleItemClicked(UObject* Item);

private:
    void RefreshList();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Mail")
    TObjectPtr<UListView> MailListView;

    FDelegateHandle OnDataChangedHandle;
    FDelegateHandle OnItemClickedHandle;
};
