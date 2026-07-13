#include "MailInboxWidget.h"
#include "SampleContents/PAGameContentModelManager.h"
#include "MailContentModel.h"
#include "Components/ListView.h"

namespace PAContentUITag
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Mail_Inbox, "UI.Mail.Inbox", "Mail inbox UI id");
}

void UMailInboxWidget::NativeOnActivated()
{
    Super::NativeOnActivated();

    if (MailListView)
    {
        OnItemClickedHandle = MailListView->OnItemClicked().AddUObject(this, &UMailInboxWidget::HandleItemClicked);
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UMailContentModel* Mail = Contents ? Contents->GetModel<UMailContentModel>() : nullptr;
    if (Mail)
    {
        OnDataChangedHandle = Mail->OnDataChanged.AddUObject(this, &UMailInboxWidget::RefreshList);
		// 캐시 정보를 가지고 메일 목록 구성
        RefreshList();
        // 메일 목록 캐시가 유효 한지 갱신
        Mail->RefreshCacheIfNeeded();
    }
}

void UMailInboxWidget::NativeOnDeactivated()
{
    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UMailContentModel* Mail = Contents ? Contents->GetModel<UMailContentModel>() : nullptr;
    if (Mail && OnDataChangedHandle.IsValid())
    {
        Mail->OnDataChanged.Remove(OnDataChangedHandle);
        OnDataChangedHandle.Reset();
    }

    if (MailListView && OnItemClickedHandle.IsValid())
    {
        MailListView->OnItemClicked().Remove(OnItemClickedHandle);
        OnItemClickedHandle.Reset();
    }

    Super::NativeOnDeactivated();
}

void UMailInboxWidget::NativeDestruct()
{
    // Deactivate 없이 파괴되는 경로(레벨 전환 등)에서 모델 구독 정리
    if (OnDataChangedHandle.IsValid())
    {
        UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
        if (UMailContentModel* Mail = Contents ? Contents->GetModel<UMailContentModel>() : nullptr)
        {
            Mail->OnDataChanged.Remove(OnDataChangedHandle);
        }
        OnDataChangedHandle.Reset();
    }

    if (MailListView && OnItemClickedHandle.IsValid())
    {
        MailListView->OnItemClicked().Remove(OnItemClickedHandle);
        OnItemClickedHandle.Reset();
    }

    Super::NativeDestruct();
}

void UMailInboxWidget::RefreshList()
{
    if (!MailListView)
    {
        return;
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    UMailContentModel* Mail = Contents ? Contents->GetModel<UMailContentModel>() : nullptr;
    if (!Mail)
    {
        return;
    }

    TArray<UObject*> Items;
    for (const FMailData& Data : Mail->GetMails())
    {
        UMailEntryObject* Entry = NewObject<UMailEntryObject>(this);
        Entry->Mail = Data;
        Items.Add(Entry);
    }
    MailListView->SetListItems(Items);
}

void UMailInboxWidget::HandleItemClicked(UObject* Item)
{
    const UMailEntryObject* Entry = Cast<UMailEntryObject>(Item);
    if (!Entry || Entry->Mail.bRead)
    {
        return;
    }

    UPAGameContentModelManager* Contents = UPAGameContentModelManager::Get(this);
    if (UMailContentModel* Mail = Contents ? Contents->GetModel<UMailContentModel>() : nullptr)
    {
        Mail->RequestRead(Entry->Mail.MailId);
    }
}
