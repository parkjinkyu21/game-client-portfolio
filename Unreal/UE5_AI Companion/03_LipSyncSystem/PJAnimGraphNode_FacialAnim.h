// AI Character Companion Application
// Built with Unreal Engine 5

// NOTE: AnimGraph 노드 클래스는 Editor 전용이므로 Developer 또는 UncookedOnly 모듈에 배치해야 합니다.
//       Runtime 모듈에 포함하면 패키징 시 에디터 의존성 문제가 발생합니다.

#pragma once
#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "PJAnimNode_FacialPoseHandler.h"
#include "PJAnimGraphNode_FacialAnim.generated.h"

/**
 * 페이셜 애님그래프 노드 (에디터 전용)
 *
 * 애니메이션 블루프린트의 AnimGraph에서 사용하는 에디터 노드.
 * FPJAnimNode_FacialPoseHandler를 내부 노드로 래핑하여
 * 블루프린트 에디터에서 페이셜 포즈 블렌딩 노드를 배치할 수 있게 한다.
 */
UCLASS(MinimalAPI)
class UPJAnimGraphNode_FacialAnim : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FPJAnimNode_FacialPoseHandler Node;

public:
	UPJAnimGraphNode_FacialAnim();

	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
};
