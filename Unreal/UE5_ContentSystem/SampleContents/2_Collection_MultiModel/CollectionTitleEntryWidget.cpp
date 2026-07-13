#include "CollectionTitleEntryWidget.h"
#include "CollectionWidget.h"
#include "CommonTextBlock.h"

namespace
{
FText TitleGradeToText(ECollectionTitleGrade Grade)
{
    switch (Grade)
    {
    case ECollectionTitleGrade::Rare:   return FText::FromString(TEXT("희귀"));
    case ECollectionTitleGrade::Legacy: return FText::FromString(TEXT("전승"));
    case ECollectionTitleGrade::Unique: return FText::FromString(TEXT("유일"));
    default:                            return FText::FromString(TEXT("일반"));
    }
}
}

void UCollectionTitleEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    const UTitleEntryObject* Entry = Cast<UTitleEntryObject>(ListItemObject);
    if (!Entry)
    {
        return;
    }

    if (NameText)
    {
        NameText->SetText(Entry->Title.Name);
    }
    if (GradeText)
    {
        GradeText->SetText(TitleGradeToText(Entry->Title.Grade));
    }
    if (EquippedText)
    {
        EquippedText->SetVisibility(Entry->Title.bEquipped ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    // 미보유 타이틀은 반투명 표시
    SetRenderOpacity(Entry->Title.bOwned ? 1.0f : 0.5f);
}
