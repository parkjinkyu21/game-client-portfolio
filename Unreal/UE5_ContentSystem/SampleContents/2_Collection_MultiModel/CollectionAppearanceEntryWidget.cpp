#include "CollectionAppearanceEntryWidget.h"
#include "CollectionWidget.h"
#include "CommonTextBlock.h"

void UCollectionAppearanceEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    const UAppearanceEntryObject* Entry = Cast<UAppearanceEntryObject>(ListItemObject);
    if (!Entry)
    {
        return;
    }

    if (NameText)
    {
        NameText->SetText(Entry->Appearance.Name);
    }

    // 미수집 외형은 반투명 표시
    SetRenderOpacity(Entry->Appearance.bCollected ? 1.0f : 0.5f);
}
