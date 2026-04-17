// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJAnimNode_PoseHandler.h"
#include "Animation/AnimInstanceProxy.h"
#include "PJFacialAnimComponent.h"

static constexpr int32 DefaultVisemeCount = 15;

FPJAnimNode_FacialPoseHandler::FPJAnimNode_FacialPoseHandler()
{
	TargetPoseWeights.Init(0.0f, DefaultVisemeCount);
	CurrentPoseWeights.Init(0.0f, DefaultVisemeCount);
}

void FPJAnimNode_FacialPoseHandler::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject());
	if (!AnimInstance)
	{
		SourcePose.Initialize(Context);
		RebuildPoseList(Context.AnimInstanceProxy->GetRequiredBones(), PoseAsset);
		return;
	}

	AActor* OwnerActor = nullptr;
	if (const USkeletalMeshComponent* SkelMeshComp = AnimInstance->GetOwningComponent())
	{
		OwnerActor = SkelMeshComp->GetOwner();
	}

	FacialAnimComponent = nullptr;
	if (OwnerActor)
	{
		FacialAnimComponent = OwnerActor->FindComponentByClass<UPJFacialAnimComponent>();
	}

	PoseByNameNodes.Empty();

	if (FacialAnimComponent.IsValid())
	{
		TArray<FString> TargetNames = FacialAnimComponent->GetTargetNames();

		for (const FString& Name : TargetNames)
		{
			FAnimNode_PoseByName PoseByNameNode;
			PoseByNameNode.PoseName = FName(*Name);
			PoseByNameNodes.Add(PoseByNameNode);
		}

		TargetPoseWeights.Init(0.0f, TargetNames.Num());
		CurrentPoseWeights.Init(0.0f, TargetNames.Num());
	}

	SourcePose.Initialize(Context);
	RebuildPoseList(Context.AnimInstanceProxy->GetRequiredBones(), PoseAsset);
}

void FPJAnimNode_FacialPoseHandler::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	SourcePose.CacheBones(Context);
}

void FPJAnimNode_FacialPoseHandler::Evaluate_AnyThread(FPoseContext& Output)
{
	SourcePose.Evaluate(Output);

	if (CurrentPoseWeights.Num() != PoseByNameNodes.Num())
	{
		return;
	}

	const UPoseAsset* CachedPoseAsset = CurrentPoseAsset.Get();
	if (CachedPoseAsset && PoseExtractContext.PoseCurves.Num() == PoseByNameNodes.Num())
	{
		for (int32 Index = 0; Index < PoseByNameNodes.Num(); ++Index)
		{
			PoseExtractContext.PoseCurves[Index].Value = CurrentPoseWeights[Index];
		}

		FAnimationPoseData OutputAnimationPoseData(Output);
		CurrentPoseAsset->GetAnimationPose(OutputAnimationPoseData, PoseExtractContext);
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FPJAnimNode_FacialPoseHandler::GatherDebugData(FNodeDebugData& DebugData)
{
	FAnimNode_Base::GatherDebugData(DebugData);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}

void FPJAnimNode_FacialPoseHandler::RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset)
{
	if (!InPoseAsset)
	{
		return;
	}

	const TArray<FName>& PoseNames = InPoseAsset->GetPoseFNames();
	if (PoseNames.Num() == 0)
	{
		return;
	}

	PoseExtractContext.PoseCurves.Reset();

	for (const FAnimNode_PoseByName& PoseByNameNode : PoseByNameNodes)
	{
		const int32 PoseIndex = InPoseAsset->GetPoseIndexByName(PoseByNameNode.PoseName);
		if (PoseIndex != INDEX_NONE)
		{
			if (PoseNames.IsValidIndex(PoseIndex))
			{
				PoseExtractContext.PoseCurves.Add(FPoseCurve(PoseIndex, PoseNames[PoseIndex], 0.f));
			}
		}
	}
}

void FPJAnimNode_FacialPoseHandler::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	FAnimNode_PoseHandler::UpdateAssetPlayer(Context);
	SourcePose.Update(Context);

	if (PoseExtractContext.PoseCurves.Num() == 0)
	{
		RebuildPoseList(Context.AnimInstanceProxy->GetRequiredBones(), PoseAsset);
	}

	TArray<float> NewTargetValues;
	if (FacialAnimComponent.IsValid())
	{
		NewTargetValues = FacialAnimComponent->GetTargetValues();
	}

	if (NewTargetValues.Num() == PoseByNameNodes.Num())
	{
		bool bValueChanged = false;
		for (int32 Index = 0; Index < PoseByNameNodes.Num(); ++Index)
		{
			if (FMath::Abs(NewTargetValues[Index] - TargetPoseWeights[Index]) > KINDA_SMALL_NUMBER)
			{
				bValueChanged = true;
				break;
			}
		}

		if (bValueChanged)
		{
			TargetPoseWeights = MoveTemp(NewTargetValues);
			SilenceTimer = SilenceCheckTime;
		}
		else
		{
			SilenceTimer -= Context.GetDeltaTime();

			if (SilenceTimer <= 0.f)
			{
				TargetPoseWeights.Init(0.0f, PoseByNameNodes.Num());
			}
		}
	}

	// 현재 가중치를 타겟으로 보간
	const float DeltaTime = Context.GetDeltaTime();
	for (int32 Index = 0; Index < PoseByNameNodes.Num(); ++Index)
	{
		CurrentPoseWeights[Index] = FMath::FInterpTo(
			CurrentPoseWeights[Index],
			TargetPoseWeights[Index],
			DeltaTime,
			InterpolationSpeed
		);
	}
}
