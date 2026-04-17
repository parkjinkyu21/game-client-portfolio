// AI Character Companion Application
// Built with Unreal Engine 5

#pragma once

#include "CoreMinimal.h"
#include "AnimNodes/AnimNode_PoseByName.h"
#include "PJAnimNode_FacialPoseHandler.generated.h"

/**
 * 페이셜 포즈 블렌딩 애님 노드
 *
 * PoseAsset에 정의된 비짐(Viseme) 포즈들을 실시간 가중치로 블렌딩하여
 * 캐릭터 페이셜 애니메이션을 구동하는 런타임 애님 노드.
 *
 * [동작 흐름]
 * 1. Initialize: 소유 액터에서 UPJFacialAnimComponent를 탐색하여 캐싱
 * 2. UpdateAssetPlayer: 페이셜 컴포넌트로부터 타겟 가중치를 매 프레임 수신
 *    - 값 변화 감지 시 타겟 가중치 갱신 및 침묵 타이머 리셋
 *    - SilenceCheckTime 동안 변화 없으면 타겟 가중치를 0으로 리셋
 * 3. Evaluate: InterpolationSpeed로 현재→타겟 가중치를 보간하여 포즈에 적용
 */
USTRUCT(BlueprintType)
struct PJCORE_API FPJAnimNode_FacialPoseHandler : public FAnimNode_PoseHandler
{
	GENERATED_BODY()

public:
	FPJAnimNode_FacialPoseHandler();

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;

	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;

	TArray<FAnimNode_PoseByName> PoseByNameNodes;

	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = Links)
	FPoseLink SourcePose;

	// 현재 → 타겟 가중치 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float InterpolationSpeed = 25.0f;

	// 비짐 값 변화 없이 이 시간이 경과하면 포즈를 0으로 리셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float SilenceCheckTime = 0.5f;

private:
	TWeakObjectPtr<class UPJFacialAnimComponent> FacialAnimComponent;
	TArray<float> TargetPoseWeights;
	TArray<float> CurrentPoseWeights;
	float SilenceTimer = 0.0f;
};
