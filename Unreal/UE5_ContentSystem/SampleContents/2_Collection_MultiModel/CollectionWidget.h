#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "NativeGameplayTags.h"
#include "CollectionContentMessages.h"
#include "CollectionWidget.generated.h"

class UButton;
class UCommonTextBlock;
class UListView;
class UWidgetSwitcher;
struct FCollectionProgressChanged;

// PAContentUITag 네임스페이스는 모든 UI가 공유한다 — 각각의 UI 클래스에서 UI id 태그를 정의 하자.
namespace PAContentUITag
{
    PACONTENTSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Collection_Main);
}

// 외형 ListView 항목 오브젝트
UCLASS()
class PACONTENTSYSTEM_API UAppearanceEntryObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "PA|Collection")
    FAppearanceEntry Appearance;
};

// 타이틀 ListView 항목 오브젝트
UCLASS()
class PACONTENTSYSTEM_API UTitleEntryObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "PA|Collection")
    FTitleEntry Title;
};

// 컬렉션 화면 위젯 — 상단 진행 요약 + 외형/타이틀 탭 전환 목록
UCLASS(Abstract)
class PACONTENTSYSTEM_API UCollectionWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void HandleAppearanceTabClicked();

    UFUNCTION()
    void HandleTitleTabClicked();

    void HandleAppearanceItemClicked(UObject* Item);
    void HandleTitleItemClicked(UObject* Item);

private:
    void UnsubscribeAll();

    void OnProgressChanged(const FCollectionProgressChanged& Event);
    void RefreshSummaryFromModel(); // 진입 시 초기값 — 부모 모델에서 직접 읽는다
    void SetSummaryText(int32 AppearanceCollected, int32 AppearanceTotal, int32 TitleOwned, int32 TitleTotal);

    void RefreshAppearanceList();
    void RefreshTitleList();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> AppearanceProgressText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UCommonTextBlock> TitleProgressText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UButton> AppearanceTabButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UButton> TitleTabButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UWidgetSwitcher> TabSwitcher;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UListView> AppearanceListView;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "PA|Collection")
    TObjectPtr<UListView> TitleListView;

    FDelegateHandle ProgressEventHandle;
    FDelegateHandle AppearanceDataHandle;
    FDelegateHandle TitleDataHandle;
    FDelegateHandle AppearanceClickHandle;
    FDelegateHandle TitleClickHandle;
};
