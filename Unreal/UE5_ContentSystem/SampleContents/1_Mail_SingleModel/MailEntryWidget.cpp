#include "MailEntryWidget.h"
#include "MailInboxWidget.h"
#include "CommonTextBlock.h"

void UMailEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    const UMailEntryObject* Entry = Cast<UMailEntryObject>(ListItemObject);
    if (!Entry)
    {
        return;
    }

    if (TitleText)
    {
        TitleText->SetText(Entry->Mail.Title);
    }

    // 읽은 메일은 반투명 표시
    SetRenderOpacity(Entry->Mail.bRead ? 0.5f : 1.0f);
}
