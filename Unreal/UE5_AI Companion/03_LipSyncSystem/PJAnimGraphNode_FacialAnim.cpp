// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJAnimGraphNode_FacialAnim.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_FacialAnim"

UPJAnimGraphNode_FacialAnim::UPJAnimGraphNode_FacialAnim()
{
}

FText UPJAnimGraphNode_FacialAnim::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UPJAnimGraphNode_FacialAnim::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PJAnimGraphNodeFacialAnim_Title", "Facial Animation");
}

FText UPJAnimGraphNode_FacialAnim::GetMenuCategory() const
{
	return LOCTEXT("PJAnimGraphNodeFacialAnim_Category", "Animation|FacialAnim");
}

#undef LOCTEXT_NAMESPACE
