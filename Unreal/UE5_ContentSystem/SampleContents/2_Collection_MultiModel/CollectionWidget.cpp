#include "CollectionWidget.h"
#include "SampleContents/PAGameContentModelManager.h"
#include "CollectionContentModel.h"
#include "CollectionAppearanceModel.h"
#include "CollectionTitleModel.h"
#include "CollectionContentEvents.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/WidgetSwitcher.h"

namespace PAContentUITag
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Collection_Main, "UI.Collection.Main", "Collection UI id");
}

void UCollectionWidget::NativeOnActivated()
{
    Super::NativeOnActivated();

    if (AppearanceTabButton)
    {
        AppearanceTabButton->OnClicked.AddUniqueDynamic(this, &UCollectionWidget::HandleAppearanceTabClicked);
    }
    if (TitleTabButton)
    {
        TitleTabButton->OnClicked.AddUniqueDynamic(this, &UCollectionWidget::HandleTitleTabClicked);
    }
    if (AppearanceListView)
    {
        AppearanceClickHandle = AppearanceListView->OnItemClicked().AddUObject(this, &UCollectionWidget::HandleAppearanceItemClicked);
    }
    if (TitleListView)
    {
        TitleClickHandle = TitleListView->OnItemClicked().AddUObject(this, &UCollectionWidget::HandleTitleItemClicked);
    }

    // 도감 진행 상태 구독
    ProgressEventHandle = UPAGameContentModelManager::RegisterContentEvent(
        this, PAContentEvents::Collection_Event_Progress, this, &UCollectionWidget::OnProgressChanged);

    // 도감 요약 정보 설정
    RefreshSummaryFromModel();

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UCollectionAppearanceModel* Appearance = Contents ? Contents->GetModel<UCollectionAppearanceModel>() : nullptr;
    if (Appearance)
    {
        AppearanceDataHandle = Appearance->OnDataChanged.AddUObject(this, &UCollectionWidget::RefreshAppearanceList);
        // 캐시 정보를 가지고 외형 도감 설정
        RefreshAppearanceList();
        // 외형 도감 캐시가 유효 한지 갱신
        Appearance->RefreshCacheIfNeeded();
    }
    UCollectionTitleModel* Title = Contents ? Contents->GetModel<UCollectionTitleModel>() : nullptr;
    if (Title)
    {
        TitleDataHandle = Title->OnDataChanged.AddUObject(this, &UCollectionWidget::RefreshTitleList);
        // 캐시 정보를 가지고 타이틀 도감 설정
        RefreshTitleList();
        // 타이틀 도감 캐시가 유효 한지 갱신
        Title->RefreshCacheIfNeeded();
    }
}

void UCollectionWidget::NativeOnDeactivated()
{
    UnsubscribeAll();
    Super::NativeOnDeactivated();
}

void UCollectionWidget::NativeDestruct()
{
    UnsubscribeAll();
    Super::NativeDestruct();
}

void UCollectionWidget::UnsubscribeAll()
{
    if (ProgressEventHandle.IsValid())
    {
        UPAGameContentModelManager::UnregisterContentEvent(this, PAContentEvents::Collection_Event_Progress, ProgressEventHandle);
        ProgressEventHandle.Reset();
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    if (AppearanceDataHandle.IsValid())
    {
        if (UCollectionAppearanceModel* Appearance = Contents ? Contents->GetModel<UCollectionAppearanceModel>() : nullptr)
        {
            Appearance->OnDataChanged.Remove(AppearanceDataHandle);
        }
        AppearanceDataHandle.Reset();
    }
    if (TitleDataHandle.IsValid())
    {
        if (UCollectionTitleModel* Title = Contents ? Contents->GetModel<UCollectionTitleModel>() : nullptr)
        {
            Title->OnDataChanged.Remove(TitleDataHandle);
        }
        TitleDataHandle.Reset();
    }

    if (AppearanceListView && AppearanceClickHandle.IsValid())
    {
        AppearanceListView->OnItemClicked().Remove(AppearanceClickHandle);
        AppearanceClickHandle.Reset();
    }
    if (TitleListView && TitleClickHandle.IsValid())
    {
        TitleListView->OnItemClicked().Remove(TitleClickHandle);
        TitleClickHandle.Reset();
    }

    if (AppearanceTabButton)
    {
        AppearanceTabButton->OnClicked.RemoveDynamic(this, &UCollectionWidget::HandleAppearanceTabClicked);
    }
    if (TitleTabButton)
    {
        TitleTabButton->OnClicked.RemoveDynamic(this, &UCollectionWidget::HandleTitleTabClicked);
    }
}

void UCollectionWidget::HandleAppearanceTabClicked()
{
    if (TabSwitcher)
    {
        TabSwitcher->SetActiveWidgetIndex(0); // WBP 스위처 : 0=외형 목록
    }
}

void UCollectionWidget::HandleTitleTabClicked()
{
    if (TabSwitcher)
    {
        TabSwitcher->SetActiveWidgetIndex(1); // WBP 스위처 : 1=타이틀 목록
    }
}

void UCollectionWidget::OnProgressChanged(const FCollectionProgressChanged& Event)
{
    SetSummaryText(Event.AppearanceCollected, Event.AppearanceTotal, Event.TitleOwned, Event.TitleTotal);
}

void UCollectionWidget::RefreshSummaryFromModel()
{
    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    if (const UCollectionContentModel* Collection = Contents ? Contents->GetModel<UCollectionContentModel>() : nullptr)
    {
        SetSummaryText(Collection->GetAppearanceCollected(), Collection->GetAppearanceTotal(),
            Collection->GetTitleOwned(), Collection->GetTitleTotal());
    }
}

void UCollectionWidget::SetSummaryText(int32 AppearanceCollected, int32 AppearanceTotal, int32 TitleOwned, int32 TitleTotal)
{
    // "n/m" 수치만
    if (AppearanceProgressText)
    {
        AppearanceProgressText->SetText(FText::AsCultureInvariant(FString::Printf(TEXT("%d/%d"), AppearanceCollected, AppearanceTotal)));
    }
    if (TitleProgressText)
    {
        TitleProgressText->SetText(FText::AsCultureInvariant(FString::Printf(TEXT("%d/%d"), TitleOwned, TitleTotal)));
    }
}

void UCollectionWidget::RefreshAppearanceList()
{
    if (!AppearanceListView)
    {
        return;
    }
    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UCollectionAppearanceModel* Appearance = Contents ? Contents->GetModel<UCollectionAppearanceModel>() : nullptr;
    if (!Appearance)
    {
        return;
    }

    TArray<UObject*> Items;
    for (const FAppearanceEntry& Data : Appearance->GetAppearances())
    {
        UAppearanceEntryObject* Entry = NewObject<UAppearanceEntryObject>(this);
        Entry->Appearance = Data;
        Items.Add(Entry);
    }
    AppearanceListView->SetListItems(Items);
}

void UCollectionWidget::RefreshTitleList()
{
    if (!TitleListView)
    {
        return;
    }
    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UCollectionTitleModel* Title = Contents ? Contents->GetModel<UCollectionTitleModel>() : nullptr;
    if (!Title)
    {
        return;
    }

    TArray<UObject*> Items;
    for (const FTitleEntry& Data : Title->GetTitles())
    {
        UTitleEntryObject* Entry = NewObject<UTitleEntryObject>(this);
        Entry->Title = Data;
        Items.Add(Entry);
    }
    TitleListView->SetListItems(Items);
}

void UCollectionWidget::HandleAppearanceItemClicked(UObject* Item)
{
    const UAppearanceEntryObject* Entry = Cast<UAppearanceEntryObject>(Item);
    if (!Entry || Entry->Appearance.bCollected)
    {
        return;
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    if (UCollectionAppearanceModel* Appearance = Contents ? Contents->GetModel<UCollectionAppearanceModel>() : nullptr)
    {
        Appearance->RequestRegister(Entry->Appearance.AppearanceId);
    }
}

void UCollectionWidget::HandleTitleItemClicked(UObject* Item)
{
    const UTitleEntryObject* Entry = Cast<UTitleEntryObject>(Item);
    if (!Entry || !Entry->Title.bOwned || Entry->Title.bEquipped)
    {
        return;
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    if (UCollectionTitleModel* Title = Contents ? Contents->GetModel<UCollectionTitleModel>() : nullptr)
    {
        Title->RequestEquip(Entry->Title.TitleId);
    }
}
