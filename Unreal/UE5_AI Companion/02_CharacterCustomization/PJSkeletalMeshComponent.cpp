// AI Character Companion Application
// Built with Unreal Engine 5


#include "PJSkeletalMeshComponent.h"


DEFINE_LOG_CATEGORY(PJSkeletalMeshComponent);

void UPJSkeletalMeshComponent::SetIgnoreMasterPose(bool bIgnore)
{
	IgnoreMasterpose = bIgnore;
}

void UPJSkeletalMeshComponent::SetMasterComponent(USkeletalMeshComponent* Comp, bool bForceUpdate, bool bInFollowerShouldTickPose)
{
	MasterComponent = Comp;

	if (!IgnoreMasterpose)
		SetLeaderPoseComponent(Comp, bForceUpdate, bInFollowerShouldTickPose);

	SetupAttachment(Comp);
}

void UPJSkeletalMeshComponent::LoadMesh(FSoftObjectPath ToLoad, int32 ItemID)
{
	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnPreLoadSkeletalMesh(this);

	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		ResourceManager->ASyncLoadObject(ToLoad, FStreamLoadCompleteDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleted, ItemID));
	}
	else
	{
		FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
		TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.RequestSyncLoad(ToLoad);
		UObject* LoadedObject = StreamableHandle.IsValid() ? StreamableHandle->GetLoadedAsset() : nullptr;
		OnLoadCompleted(LoadedObject, ItemID);
	}
}

void UPJSkeletalMeshComponent::LoadMaterial(FSoftObjectPath ToLoad, FString SlotName)
{
	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		ResourceManager->ASyncLoadObject(ToLoad, FStreamLoadCompleteDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleteMaterial, SlotName));
	}
	else
	{
		UE_LOG(PJSkeletalMeshComponent, Log, TEXT("Can not found Resource manager."));
	}
}

void UPJSkeletalMeshComponent::RemoveMesh()
{
	LoadMesh(FSoftObjectPath(TEXT("StaticMesh'/Game/CommonAssets/Meshes/Dummy'")), 0);
	MySlicerName.Empty();
}

// Sync 로딩 — 모션 동기화가 필요한 경우 사용
void UPJSkeletalMeshComponent::SyncLoadMeshAnim(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadAnim)
{
	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		OriginalSkeletalMesh = Cast<USkeletalMesh>(ResourceManager->SyncLoadObject(ToLoadMesh, EPJAssetCacheType::PJS_InfinityTime));
		if (OriginalSkeletalMesh)
		{
			IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
			if (CostumeInterface)
				CostumeInterface->OnPreLoadSkeletalMesh(this);

			SetSkeletalMesh(OriginalSkeletalMesh);

			if (CostumeInterface)
				CostumeInterface->OnSkeletalLoadCompleted(this, -1);

			UpdateIgnoreMasterPose();

			UClass* AnimClass = Cast<UClass>(ResourceManager->SyncLoadObject(ToLoadAnim, EPJAssetCacheType::PJS_InfinityTime));
			if (AnimClass)
			{
				SetAnimationMode(EAnimationMode::AnimationBlueprint);
				SetAnimInstanceClass(AnimClass);
			}

			if (CostumeInterface)
				CostumeInterface->OnAnimLoadCompleted(this);
		}
	}
}

void UPJSkeletalMeshComponent::AsyncLoadMeshAnim(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadAnim)
{
	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnPreLoadSkeletalMesh(this);

	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		TArray<FSoftObjectPath> AssetPaths;
		AssetPaths.Add(ToLoadMesh);
		AssetPaths.Add(ToLoadAnim);
		ResourceManager->ASyncLoadObjects(AssetPaths, FStreamLoadCompletesDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleteMeshAnim, AssetPaths));
	}
	else
	{
		FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();

		TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.RequestSyncLoad(ToLoadMesh);
		UObject* LoadedObject = StreamableHandle.IsValid() ? StreamableHandle->GetLoadedAsset() : nullptr;
		OnLoadCompleted(LoadedObject, -1);

		StreamableHandle = StreamableManager.RequestSyncLoad(ToLoadAnim);
		LoadedObject = StreamableHandle.IsValid() ? StreamableHandle->GetLoadedAsset() : nullptr;
		OnLoadCompleteAnim(LoadedObject);
	}
}

void UPJSkeletalMeshComponent::AsyncLoadMeshMaterial(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadMat, FString SlotName, int32 ItemID)
{
	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnPreLoadSkeletalMesh(this);

	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		TArray<FSoftObjectPath> AssetPaths;
		AssetPaths.Add(ToLoadMesh);
		AssetPaths.Add(ToLoadMat);
		ResourceManager->ASyncLoadObjects(AssetPaths, FStreamLoadCompletesDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleteMeshMaterial, SlotName, AssetPaths, ItemID));
	}
	else
	{
		UE_LOG(PJSkeletalMeshComponent, Log, TEXT("Can not found Resource manager."));
	}
}

void UPJSkeletalMeshComponent::AsyncLoadAnim(FSoftObjectPath ToLoadAnim)
{
	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		ResourceManager->ASyncLoadObject(ToLoadAnim, FStreamLoadCompleteDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleteAnim));
	}
}

class UMaterialInstanceDynamic* UPJSkeletalMeshComponent::FindCustomMaterial(FString Name)
{
	int32 Index = GetMaterialIndex(FName(*Name));
	if (Index == INDEX_NONE)
		return nullptr;

	for (auto& MaterialEntry : CustomMaterialDatas)
	{
		if (MaterialEntry.MaterialSlotIndex == Index)
		{
			return MaterialEntry.InstanceMaterial.Get();
		}
	}

	return nullptr;
}

void UPJSkeletalMeshComponent::SetSkeletalMesh(class USkeletalMesh* NewMesh, bool bReinitPose /*= true*/)
{
	Super::SetSkeletalMesh(NewMesh, bReinitPose);
	for (auto cloneMeshComponent : CloneMeshComponents)
	{
		if (IsValid(cloneMeshComponent))
		{
			cloneMeshComponent->SetSkeletalMesh(NewMesh, bReinitPose);
		}
	}
}

void UPJSkeletalMeshComponent::SetPhysicsAsset(class UPhysicsAsset* NewPhysicsAsset, bool bForceReInit /*= false*/)
{
	Super::SetPhysicsAsset(NewPhysicsAsset, bForceReInit);
	for (auto cloneMeshComponent : CloneMeshComponents)
	{
		if (IsValid(cloneMeshComponent))
			cloneMeshComponent->SetPhysicsAsset(NewPhysicsAsset, bForceReInit);
	}
}

void UPJSkeletalMeshComponent::UpdateFollowerComponent()
{
	// 부모의 모프타겟을 자식에게 그대로 전파하지 않고, 자체 모프타겟만 적용
	ActiveMorphTargets.Reset();

	if (GetSkeletalMeshAsset())
	{
		MorphTargetWeights.SetNum(GetSkeletalMeshAsset()->GetMorphTargets().Num());

		if (MorphTargetWeights.Num() > 0)
		{
			FMemory::Memzero(MorphTargetWeights.GetData(), MorphTargetWeights.GetAllocatedSize());
		}
	}
	else
	{
		MorphTargetWeights.Reset();
	}

	if (USkeletalMeshComponent* LeaderSMC = Cast<USkeletalMeshComponent>(LeaderPoseComponent.Get()))
	{
		if (GetSkeletalMeshAsset())
		{
			// Follower 자체의 모프타겟 커브만 적용 (Leader 것은 전파하지 않음)
			if (GetMorphTargetCurves().Num() > 0)
			{
				FAnimationRuntime::AppendActiveMorphTargets(GetSkeletalMeshAsset(), GetMorphTargetCurves(), ActiveMorphTargets, MorphTargetWeights);
			}
		}
	}

	USkinnedMeshComponent::UpdateFollowerComponent();
}

void UPJSkeletalMeshComponent::AddCloneMeshComponent(UPJSkeletalMeshComponent* Clone, bool bUpdateLocation)
{
	RemoveCloneMeshComponent(Clone);

	if (Clone)
	{
		CloneMeshComponents.Add(Clone);
		Clone->SetSkeletalMesh(GetSkeletalMesh());
		if (bUpdateLocation)
			Clone->SetRelativeLocation(GetRelativeLocation());
		Clone->ClearMorphTargets();

		const TMap<FName, float>& Targets = GetMorphTargetCurves();
		for (auto MorphTarget : Targets)
		{
			Clone->SetMorphTarget(MorphTarget.Key, MorphTarget.Value);
		}
		Clone->MirroringMaterialFromComponent(this);
	}
}

void UPJSkeletalMeshComponent::RemoveCloneMeshComponent(UPJSkeletalMeshComponent* Clone)
{
	CloneMeshComponents.Remove(Clone);
}

void UPJSkeletalMeshComponent::ClearMorphTargetsWithChild()
{
	ClearMorphTargets();
	for (auto cloneMeshComponent : CloneMeshComponents)
	{
		if (IsValid(cloneMeshComponent))
			cloneMeshComponent->ClearMorphTargets();
	}
}

void UPJSkeletalMeshComponent::SetMorphTargetWithChild(FName MorphTargetName, float Value, bool bRemoveZeroWeight /*= true*/)
{
	SetMorphTarget(MorphTargetName, Value, bRemoveZeroWeight);
	for (auto cloneMeshComponent : CloneMeshComponents)
	{
		if (IsValid(cloneMeshComponent))
			cloneMeshComponent->SetMorphTarget(MorphTargetName, Value, bRemoveZeroWeight);
	}
}

void UPJSkeletalMeshComponent::SetClothMorphTarget(FString SlotPrefix, const FPJClothMorphData& MorphData)
{
	ClearMorphTargets();

	for (TFieldIterator<FProperty> it(FPJClothMorphData::StaticStruct()); it; ++it)
	{
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(*it))
		{
			const FString PropertyName = FloatProp->GetName();

			if (PropertyName.StartsWith(SlotPrefix))
			{
				FString TargetName = PropertyName.RightChop(SlotPrefix.Len());
				float MorphValue = FloatProp->GetPropertyValue_InContainer(&MorphData);
				SetMorphTarget(*TargetName, MorphValue);
				UE_LOG(PJSkeletalMeshComponent, Log, TEXT("SetClothMorphTarget SlotName[%s] MorphTargetName[%s] MorphTargetValue[%.2f]"), *SlotPrefix, *TargetName, MorphValue);
			}
		}
	}
}

void UPJSkeletalMeshComponent::ResetSlicerData()
{
	MySlicerName.Empty();
}

void UPJSkeletalMeshComponent::SetSlicerData(FString& Name)
{
	MySlicerName = Name;
}

void UPJSkeletalMeshComponent::OnLoadCompleted(UObject* Loaded, int32 ItemID)
{
	ResetMaterialData(true);

	if (ItemID == 0)
	{
		OriginalSkeletalMesh = nullptr;
		SetSkeletalMesh(nullptr);
	}
	else
	{
		SetPhysicsAsset(nullptr);
		OriginalSkeletalMesh = Cast<USkeletalMesh>(Loaded);
		SetSkeletalMesh(OriginalSkeletalMesh);
	}

	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnSkeletalLoadCompleted(this, ItemID);

	if (Loaded == nullptr)
		return;

	if (bHasDeferredMaterial)
	{
		OnLoadCompleteMaterial(DeferredMaterial, DeferredSlotName);
	}

	TArray<FPJMaterialDatas> TempList(DeferredMaterialData);
	for (FPJMaterialDatas& MatData : TempList)
	{
		ChangeMaterialData(MatData, true);
	}

	DeferredMaterialData.Empty();
}

void UPJSkeletalMeshComponent::OnLoadCompleteTextureParam(UObject* Loaded, TWeakObjectPtr<UMaterialInstanceDynamic> Material, FString ParamName)
{
	if (CustomMaterialDatas.IsEmpty())
		return;

	bool bMatched = false;
	for (auto& MaterialEntry : CustomMaterialDatas)
	{
		if (MaterialEntry.InstanceMaterial == Material)
			bMatched = true;
	}

	UTexture* Tex = Cast<UTexture>(Loaded);
	if (Material.IsValid() && bMatched)
	{
		Material->SetTextureParameterValue(*ParamName, Tex);
	}

	if (!bMatched)
	{
		UE_LOG(PJSkeletalMeshComponent, Log, TEXT("Is not matched material instance."));
	}
}

void UPJSkeletalMeshComponent::OnLoadCompleteMaterial(UObject* Loaded, FString SlotName)
{
	UMaterialInstance* Mat = Cast<UMaterialInstance>(Loaded);

	int32 SlotIdx = GetMaterialIndex(FName(*SlotName));

	// 메시 로딩이 끝나기 전에 머티리얼이 먼저 로딩된 경우 지연 적용
	if (SlotIdx == -1)
	{
		bHasDeferredMaterial = true;
		DeferredSlotName = SlotName;
		DeferredMaterial = Mat;
		return;
	}
	else
	{
		bHasDeferredMaterial = false;
		DeferredMaterial = nullptr;
	}

	bool bExist = false;
	// 이미 같은 슬롯의 머티리얼이 있다면 재적용
	for (auto& MaterialEntry : CustomMaterialDatas)
	{
		if (MaterialEntry.MaterialSlotIndex == SlotIdx)
		{
			MaterialEntry.OriginalMaterial = Mat;
			MaterialEntry.InstanceMaterial = CreateAndSetMaterialInstanceDynamicFromMaterial(SlotIdx, Mat);
			for (auto& ParamData : MaterialEntry.MaterialParamDatas)
			{
				ChangeMaterialData(ParamData, true);
			}
			bExist = true;
			break;
		}
	}
	if (!bExist)
	{
		FCustomMaterialData NewMatData;
		NewMatData.MaterialSlotIndex = SlotIdx;
		NewMatData.OriginalMaterial = Mat;
		NewMatData.InstanceMaterial = CreateAndSetMaterialInstanceDynamicFromMaterial(SlotIdx, Mat);
		CustomMaterialDatas.Add(NewMatData);
	}

	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnMaterialLoadComplete(this);

	for (auto cloneMeshComponent : CloneMeshComponents)
	{
		if (IsValid(cloneMeshComponent))
		{
			cloneMeshComponent->MirroringMaterialFromComponent(this);
		}
	}
}

void UPJSkeletalMeshComponent::OnLoadCompleteAnim(UObject* Loaded)
{
	UClass* AnimClass = Cast<UClass>(Loaded);
	if (AnimClass)
	{
		if (GetAnimClass() != AnimClass)
		{
			UpdateIgnoreMasterPose();
			SetAnimationMode(EAnimationMode::AnimationBlueprint);
			SetAnimInstanceClass(AnimClass);

			if (UAnimInstance* AnimInst = GetAnimInstance())
				AnimInst->NativeBeginPlay();
		}
	}

	IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
	if (CostumeInterface)
		CostumeInterface->OnAnimLoadCompleted(this);
}

void UPJSkeletalMeshComponent::OnLoadCompleteMeshAnim(TArray<UObject*> Loaded, TArray<FSoftObjectPath> Paths)
{
	TArray<UObject*> Temp = UPJResourceManager::GetSortLoadedObjects(Loaded, Paths);

	OnLoadCompleted(Temp[0], -1);
	OnLoadCompleteAnim(Temp[1]);
}

void UPJSkeletalMeshComponent::OnLoadCompleteMeshMaterial(TArray<UObject*> Loaded, FString SlotName, TArray<FSoftObjectPath> Paths, int32 ItemID)
{
	TArray<UObject*> Temp = UPJResourceManager::GetSortLoadedObjects(Loaded, Paths);

	if (ItemID == -1)
	{
		// body: 머티리얼 지연 적용 후 메시 로딩 완료 처리
		bHasDeferredMaterial = true;
		DeferredSlotName = SlotName;
		DeferredMaterial = Cast<UMaterialInstance>(Temp[1]);

		OnLoadCompleted(Temp[0], ItemID);
	}
	else
	{
		// 옷에 머티리얼이 추가될 경우
		ResetMaterialData(true);
		if (ItemID == 0)
		{
			OriginalSkeletalMesh = nullptr;
			SetSkeletalMesh(nullptr);
		}
		else
		{
			SetPhysicsAsset(nullptr);
			OriginalSkeletalMesh = Cast<USkeletalMesh>(Temp[0]);
			SetSkeletalMesh(OriginalSkeletalMesh);
		}

		UMaterialInstance* Mat = Cast<UMaterialInstance>(Temp[1]);

		int32 MatSlotIdx = GetMaterialIndex(FName(*SlotName));
		if (MatSlotIdx == -1)
		{
			MatSlotIdx = 0;
		}

		if (Mat != 0)
		{
			FCustomMaterialData NewMatData;
			NewMatData.MaterialSlotIndex = MatSlotIdx;
			NewMatData.OriginalMaterial = Mat;
			NewMatData.InstanceMaterial = CreateAndSetMaterialInstanceDynamicFromMaterial(MatSlotIdx, Mat);
			CustomMaterialDatas.Add(NewMatData);
		}

		IPJCostumeInterface* CostumeInterface = Cast<IPJCostumeInterface>(GetOwner());
		if (CostumeInterface)
			CostumeInterface->OnSkeletalLoadCompleted(this, ItemID);
	}
}

void UPJSkeletalMeshComponent::SetAttachSocketName(FName InSocketName)
{
	SocketName = InSocketName;
	UpdateAttachSocketName();
}

void UPJSkeletalMeshComponent::UpdateAttachSocketName()
{
	if (GetOwner() && SocketName != "None")
	{
		AttachToComponent(GetAttachParent(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	}
}

#if WITH_EDITOR
void UPJSkeletalMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName ChangedPropertyName(PropertyChangedEvent.Property->GetName());

		if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UPJSkeletalMeshComponent, SocketName))
		{
			UpdateAttachSocketName();
		}
		if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UPJSkeletalMeshComponent, IgnoreMasterpose))
		{
			ChangeIgnoreMasterpose(IgnoreMasterpose);
		}
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
}
#endif

void UPJSkeletalMeshComponent::ChangeIgnoreMasterpose(bool bOut)
{
	IgnoreMasterpose = bOut;
	if (bOut)
	{
		SetLeaderPoseComponent(nullptr);
	}
	else
	{
		SetLeaderPoseComponent(MasterComponent.Get());
	}
}

void UPJSkeletalMeshComponent::UpdateIgnoreMasterPose()
{
	if (IgnoreMasterpose)
	{
		SetLeaderPoseComponent(nullptr);
	}
	else
	{
		SetLeaderPoseComponent(MasterComponent.Get());
	}
}

void UPJSkeletalMeshComponent::SetOriginalSkeletalMesh(USkeletalMesh* NewMesh)
{
	OriginalSkeletalMesh = NewMesh;
}

USkeletalMesh* UPJSkeletalMeshComponent::GetSkeletalMesh()
{
	return GetSkeletalMeshAsset();
}

USkeletalMesh* UPJSkeletalMeshComponent::GetOriginalSkeletalMesh()
{
	return OriginalSkeletalMesh;
}

void UPJSkeletalMeshComponent::MakeCloneSkeletalMesh(TArray<TSharedPtr<class FSliceConditionBase>>& OutSlicerArray, UPhysicsAsset* Physics)
{
	if (OriginalSkeletalMesh == nullptr)
	{
		OriginalSkeletalMesh = GetSkeletalMeshAsset();
	}

	if (OutSlicerArray.IsEmpty())
	{
		SetSkeletalMesh(OriginalSkeletalMesh);
		// 피직스가 없으면 렌더링 문제 발생 — body의 피직스 에셋을 할당
		if (OriginalSkeletalMesh && OriginalSkeletalMesh->GetPhysicsAsset() == nullptr && Physics != nullptr)
		{
			SetPhysicsAsset(Physics, true);
		}
		return;
	}

	if (OriginalSkeletalMesh)
	{
		TSharedPtr<FPJMakeCloneSkeltalMesh> Maker = MakeShared<FPJMakeCloneSkeltalMesh>();
		USkeletalMesh* NewClone = nullptr;
		Maker->MakeClone(NewClone, OriginalSkeletalMesh, OutSlicerArray);
		SetSkeletalMesh(NewClone);

		if (Physics != nullptr && OriginalSkeletalMesh->GetPhysicsAsset() == nullptr)
		{
			SetPhysicsAsset(Physics, true);
		}
	}
}

void UPJSkeletalMeshComponent::ChangeMaterialData(FPJMaterialDatas& Data, bool bApplyTexture)
{
	if (GetSkeletalMesh() == nullptr)
	{
		DeferredMaterialData.Add(Data);
		return;
	}

	int32 SlotIdx = GetMaterialIndex(FName(*Data.MaterialSlotName));
	if (SlotIdx == INDEX_NONE)
		return;

	auto SetParamFunc = [&](TWeakObjectPtr<UMaterialInstanceDynamic> MatInstance, bool bTexture) {

		if (MatInstance != nullptr)
		{
			if (Data.ParamType == EPJMaterialParamType::PJMP_COLOR)
			{
				MatInstance->SetVectorParameterValue(*Data.ParamName, Data.Color);
			}
			else if (Data.ParamType == EPJMaterialParamType::PJMP_FLOAT)
			{
				MatInstance->SetScalarParameterValue(*Data.ParamName, Data.Value);
			}
			else if (Data.ParamType == EPJMaterialParamType::PJMP_TEXTURE && bTexture)
			{
				UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
				if (ResourceManager)
				{
					ResourceManager->ASyncLoadObject(FSoftObjectPath(*Data.TextureAssetName), FStreamLoadCompleteDelegate::CreateUObject(this, &UPJSkeletalMeshComponent::OnLoadCompleteTextureParam, MatInstance, Data.ParamName));
				}
			}
		}
	};

	for (auto& MaterialEntry : CustomMaterialDatas)
	{
		if (MaterialEntry.MaterialSlotIndex == SlotIdx)
		{
			SetParamFunc(MaterialEntry.InstanceMaterial, bApplyTexture);
			for (auto& ParamData : MaterialEntry.MaterialParamDatas)
			{
				if (ParamData.ParamName == Data.ParamName)
				{
					ParamData = Data;
					return;
				}
			}

			MaterialEntry.MaterialParamDatas.Add(Data);
			return;
		}
	}

	FCustomMaterialData NewMatData;
	NewMatData.MaterialSlotIndex = SlotIdx;
	NewMatData.InstanceMaterial = CreateAndSetMaterialInstanceDynamic(SlotIdx);
	SetParamFunc(NewMatData.InstanceMaterial, bApplyTexture);
	NewMatData.MaterialParamDatas.Add(Data);

	CustomMaterialDatas.Add(NewMatData);
}

void UPJSkeletalMeshComponent::ResetMaterialData(bool bForce)
{
	DeferredMaterialData.Empty();
	if (bForce)
		EmptyOverrideMaterials();

	CustomMaterialDatas.Empty();
}

void UPJSkeletalMeshComponent::CopyMaterialFromComponent(UPJSkeletalMeshComponent* Original)
{
	ResetMaterialData();
	TArray<FCustomMaterialData>& OrigArray = Original->GetCustomMaterialDatas();

	for (auto& MaterialEntry : OrigArray)
	{
		FCustomMaterialData NewMatData;
		NewMatData.MaterialSlotIndex = MaterialEntry.MaterialSlotIndex;
		NewMatData.OriginalMaterial = MaterialEntry.OriginalMaterial;
		NewMatData.InstanceMaterial = CreateAndSetMaterialInstanceDynamicFromMaterial(MaterialEntry.MaterialSlotIndex, MaterialEntry.OriginalMaterial.Get());
		CustomMaterialDatas.Add(NewMatData);

		for (auto& ParamData : MaterialEntry.MaterialParamDatas)
		{
			ChangeMaterialData(ParamData, true);
		}
	}
}

void UPJSkeletalMeshComponent::MirroringMaterialFromComponent(UPJSkeletalMeshComponent* Original)
{
	ResetMaterialData();

	TArray<FName> MaterialSlotNames = Original->GetMaterialSlotNames();

	// 동일한 머티리얼 슬롯만 미러링
	for (int32 i = 0; i < MaterialSlotNames.Num(); i++)
	{
		int32 SlotIdx = GetMaterialIndex(MaterialSlotNames[i]);
		if (SlotIdx == INDEX_NONE)
		{
			continue;
		}

		UMaterialInterface* MirrorMaterial = Original->GetMaterial(i);
		if (MirrorMaterial)
		{
			SetMaterial(SlotIdx, MirrorMaterial);
		}
	}
}

void UPJSkeletalMeshComponent::ResetDatas()
{
	SetSkeletalMesh(nullptr);
	SetOriginalSkeletalMesh(nullptr);
	SetPhysicsAsset(nullptr);
	ResetSlicerData();
	ResetMaterialData(true);
	DeferredMaterial = nullptr;
	bHasDeferredMaterial = false;
}

void UPJSkeletalMeshComponent::Overlaped(bool bSelect, int32 Color)
{
	if (bSelect)
	{
		SetCustomDepthStencilValue(Color);
		SetRenderCustomDepth(true);
	}
	else
	{
		SetCustomDepthStencilValue(0);
		SetRenderCustomDepth(false);
	}
}
