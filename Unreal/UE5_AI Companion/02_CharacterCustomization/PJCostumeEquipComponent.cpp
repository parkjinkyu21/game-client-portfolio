// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJCostumeEquipComponent.h"

DEFINE_LOG_CATEGORY(PJCostumeEquipComponent);

// ═══════════════════════════════════════════════════════════════════════
//  Initialization
// ═══════════════════════════════════════════════════════════════════════

void UPJCostumeEquipComponent::InitializeSlots(USkeletalMeshComponent* RootMesh)
{
	// 부위별 소켓 스켈레탈 메시 컴포넌트 생성
	SlotComponents.Init(nullptr, (int32)EPJEquipSlotTypeEnum::MAXCOUNT);
	for (int32 i = 0; i < (int32)EPJEquipSlotTypeEnum::MAXCOUNT; i++)
	{
		FString ObjName = FString::Printf(TEXT("SKELETAL_MESH_%d"), i + 1);
		SlotComponents[i] = NewObject<UPJSkeletalMeshComponent>(GetOwner(), *ObjName);
		SlotComponents[i]->SetMasterComponent(RootMesh);
		SlotComponents[i]->SetCollisionProfileName(TEXT("NoCollision"));
		SlotComponents[i]->RegisterComponent();
		SlotComponents[i]->AddTickPrerequisiteComponent(RootMesh);
		SlotComponents[i]->SetLightingChannels(
			RootMesh->LightingChannels.bChannel0,
			RootMesh->LightingChannels.bChannel1,
			RootMesh->LightingChannels.bChannel2
		);
	}

	// HairEffect 컴포넌트 생성
	HairEffectComponent = NewObject<UPJSkeletalMeshComponent>(GetOwner(), TEXT("HairEffect"));
	HairEffectComponent->TranslucencySortPriority = 1;
	HairEffectComponent->SetMasterComponent(RootMesh);
	HairEffectComponent->SetCollisionProfileName(TEXT("NoCollision"));
	HairEffectComponent->RegisterComponent();
	HairEffectComponent->AddTickPrerequisiteComponent(RootMesh);
	HairEffectComponent->SetActive(false);

	// LODSync - Character의 생성자에서 이미 생성된 컴포넌트를 참조
	LODSyncComponent = GetOwner()->FindComponentByClass<ULODSyncComponent>();
	if (LODSyncComponent)
	{
		LODSyncComponent->ComponentsToSync.Emplace(TEXT("HairEffect"), ESyncOption::Passive);
		LODSyncComponent->RefreshSyncComponents();
	}
}

void UPJCostumeEquipComponent::CleanupSlots()
{
	RemoveAllCostume();

	for (int32 i = 0; i < (int32)EPJEquipSlotTypeEnum::MAXCOUNT; ++i)
	{
		if (SlotComponents.IsValidIndex(i) && SlotComponents[i])
		{
			SlotComponents[i]->SetHiddenInGame(false);
		}
	}
}


// ═══════════════════════════════════════════════════════════════════════
//  Mesh Loading (비동기 에셋 로딩)
// ═══════════════════════════════════════════════════════════════════════

void UPJCostumeEquipComponent::LoadSkeletalSocket(FSoftObjectPath ToLoad, EPJEquipSlotTypeEnum Part, int32 ItemID)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;
	SlotComponents[(int32)Part]->LoadMesh(ToLoad, ItemID);
}

void UPJCostumeEquipComponent::LoadMaterialSocket(FSoftObjectPath ToLoad, EPJEquipSlotTypeEnum Part, FString SlotName)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;
	SlotComponents[(int32)Part]->LoadMaterial(ToLoad, SlotName);
}

void UPJCostumeEquipComponent::LoadSkeletalMaterialSocket(FSoftObjectPath ToLoad, FSoftObjectPath ToMat, EPJEquipSlotTypeEnum Part, FString SlotName, int32 ItemID)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;
	SlotComponents[(int32)Part]->AsyncLoadMeshMaterial(ToLoad, ToMat, SlotName, ItemID);
}

void UPJCostumeEquipComponent::LoadSkeletalAnimSocket(FSoftObjectPath ToLoad, FSoftObjectPath ToAnim, EPJEquipSlotTypeEnum Part)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;
	SlotComponents[(int32)Part]->SyncLoadMeshAnim(ToLoad, ToAnim);
}

void UPJCostumeEquipComponent::SetSkeletalSocket(USkeletalMesh* Mesh, EPJEquipSlotTypeEnum Part)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;
	SlotComponents[(int32)Part]->SetSkeletalMesh(Mesh);
}

void UPJCostumeEquipComponent::RemoveSkeletalSocket(EPJEquipSlotTypeEnum Part, bool bForce)
{
	if (SlotComponents.Num() <= (int32)Part)
		return;

	if (!bForce)
	{
		if (EquipCostumeData.GetDataTID(Part) <= 0 && SlotComponents[(int32)Part]->GetSkeletalMesh() == nullptr)
		{
			SlotComponents[(int32)Part]->ResetSlicerData();
			return;
		}
	}

	LoadCostumeData.setDataTID(Part, 0, 0, 0);
	// 더미 오브젝트를 로딩하여 실제 삭제는 HandleSkeletalLoadCompleted에서 처리
	SlotComponents[(int32)Part]->RemoveMesh();
}


// ═══════════════════════════════════════════════════════════════════════
//  Costume Data (코스튬 장착 데이터)
// ═══════════════════════════════════════════════════════════════════════

void UPJCostumeEquipComponent::SetEquipCostumeData(FPJEquipCostumeData& Data, EPJGenderType GenderType)
{
	LoadCostumeData = Data;

	for (int32 EnumIndex = (int32)EPJEquipSlotTypeEnum::HEADWEAR; EnumIndex < (int32)EPJEquipSlotTypeEnum::MAXCOUNT; ++EnumIndex)
	{
		EPJEquipSlotTypeEnum CurrentSocketType = (EPJEquipSlotTypeEnum)EnumIndex;
		LoadCostumeData.setEtcData(CurrentSocketType, (int32)GenderType, 0);
		if (EquipCostumeData.IsSameSlot(LoadCostumeData, CurrentSocketType))
			continue;

		int32 CurrentItemID = LoadCostumeData.GetDataTID(CurrentSocketType);
		if (CurrentItemID <= 0)
		{
			RemoveSkeletalSocket(CurrentSocketType, true);
			continue;
		}

		const FPJItemData* ItemData = UPJDataTables::Resolve<UPJItemDataTable>(CurrentItemID);
		if (ItemData)
		{
			// 성별 불일치 시 슬롯 삭제
			if (ItemData->Gender != EPJGenderType::ALL && GenderType != ItemData->Gender)
			{
				RemoveSkeletalSocket(CurrentSocketType, true);
				continue;
			}
			LoadFromClothData(CurrentSocketType, CurrentItemID, GenderType);
		}
		else
		{
			RemoveSkeletalSocket(CurrentSocketType, true);
		}
	}
}

void UPJCostumeEquipComponent::ReEquipCostumeData(EPJGenderType GenderType)
{
	LoadCostumeData = EquipCostumeData;
	FPJEquipCostumeData EmptyData;
	EquipCostumeData = EmptyData;
	SetEquipCostumeData(LoadCostumeData, GenderType);
}

void UPJCostumeEquipComponent::LoadEquipCostumeData(EPJEquipSlotTypeEnum SocketType, int32 CostumeID, EPJGenderType GenderType)
{
	if (LoadCostumeData.GetDataTID(SocketType) == CostumeID)
		return;

	if (CostumeID == 0)
	{
		RemoveSkeletalSocket(SocketType, true);
		return;
	}

	LoadCostumeData.setDataTID(SocketType, CostumeID, (int32)GenderType, 0);
	const FPJItemData* ItemData = UPJDataTables::Resolve<UPJItemDataTable>(CostumeID);
	if (ItemData)
	{
		LoadFromClothData(SocketType, CostumeID, GenderType);
	}
	else
	{
		RemoveSkeletalSocket(SocketType, true);
	}
}

void UPJCostumeEquipComponent::SetEquipCostume(int32 CostumeID, EPJGenderType GenderType)
{
	const FPJItemData* ItemData = UPJDataTables::Resolve<UPJItemDataTable>(CostumeID);
	EPJEquipSlotTypeEnum SlotType = EPJEquipSlotTypeEnum::MAXCOUNT;
	if (UPJDataTables::Resolve<UPJClothEquipTypeDataTable>(ItemData->TypeSmall))
	{
		SlotType = UPJDataTables::Resolve<UPJClothEquipTypeDataTable>(ItemData->TypeSmall)->EquipSlotType;
	}

	if (SlotType != EPJEquipSlotTypeEnum::MAXCOUNT)
	{
		LoadCostumeData.setDataTID(SlotType, CostumeID, (int32)GenderType, 0);
		LoadFromClothData(SlotType, CostumeID, GenderType);
	}
}

void UPJCostumeEquipComponent::RemoveAll()
{
	ResetCostumeData();

	UPJResourceManager* ResourceManager = UPJResourceManager::Inst();
	if (ResourceManager)
	{
		ResourceManager->CancelLoading(GetOwner());

		for (int32 i = 0; i < (int32)SlotComponents.Num(); ++i)
		{
			ResourceManager->CancelLoading(SlotComponents[i]);
		}
	}
	LoadCostumeData = EquipCostumeData;
}

void UPJCostumeEquipComponent::RemoveAllCostume()
{
	RemoveAll();

	FPJEquipCostumeData temp;
	EquipCostumeData = temp;
	LoadCostumeData = EquipCostumeData;

	if (HairEffectComponent)
		HairEffectComponent->SetSkeletalMesh(nullptr);

	for (int i = 0; i < (int)SlotComponents.Num(); i++)
	{
		if (SlotComponents[i])
			SlotComponents[i]->ResetDatas();
	}
}

void UPJCostumeEquipComponent::ResetCostumeData()
{
	for (int i = 0; i < (int)SlotComponents.Num(); ++i)
	{
		if (SlotComponents[i])
		{
			SlotComponents[i]->ResetMaterialData(false);
		}
	}
}

bool UPJCostumeEquipComponent::IsEquipItem(EPJEquipSlotTypeEnum Part, EPJItemSmallTypeEnum TypeSmall)
{
	if (LoadCostumeData.GetDataTID(Part) != 0)
	{
		int32 CurrentItemID = LoadCostumeData.GetDataTID(Part);
		const FPJItemData* ItemData = UPJDataTables::Resolve<UPJItemDataTable>(CurrentItemID);
		if (ItemData && ItemData->TypeSmall == TypeSmall)
			return true;
	}
	return false;
}


// ═══════════════════════════════════════════════════════════════════════
//  Internal - Cloth Data Loading
// ═══════════════════════════════════════════════════════════════════════

void UPJCostumeEquipComponent::LoadFromClothData(EPJEquipSlotTypeEnum Socket, int32 ItemID, EPJGenderType GenderType)
{
	FPJClothItemData* ClothItemData = UPJDataTables::Resolve<UPJClothItemDataTable>(ItemID);
	if (ClothItemData == nullptr)
	{
		UE_LOG(PJCostumeEquipComponent, Log, TEXT("Can not found clothItemData item_id : (%d)"), ItemID);
		return;
	}

	FSoftObjectPath MeshPath = (GenderType == EPJGenderType::MALE ? ClothItemData->MeshM : ClothItemData->MeshF);
	FSoftObjectPath MatPath = (GenderType == EPJGenderType::MALE ? ClothItemData->MaterialM : ClothItemData->MaterialF);
	if (MeshPath.IsNull())
	{
		MeshPath = FSoftObjectPath(TEXT("StaticMesh'/Game/CommonAssets/Meshes/Dummy'"));
		UE_LOG(PJCostumeEquipComponent, Log, TEXT("Can not found cloth mesh data item_id : (%d)"), ItemID);
	}

	if (MatPath.IsNull())
		LoadSkeletalSocket(MeshPath, Socket, ItemID);
	else
		LoadSkeletalMaterialSocket(MeshPath, MatPath, Socket, TEXT("#main"), ItemID);
}


// ═══════════════════════════════════════════════════════════════════════
//  Load Callbacks (Character dispatches IPJCostumeInterface here)
// ═══════════════════════════════════════════════════════════════════════

int32 UPJCostumeEquipComponent::HandleSkeletalLoadCompleted(UPJSkeletalMeshComponent* Comp, USkeletalMeshComponent* RootMesh, int32 ItemID, EPJGenderType GenderType)
{
	if (Comp == RootMesh)
		return -1;

	// HairEffect 로딩 완료 처리
	if (Comp == HairEffectComponent)
	{
		SlotComponents[(int)EPJEquipSlotTypeEnum::HAIR]->SetHiddenInGame(false);
		SlotComponents[(int)EPJEquipSlotTypeEnum::HEADWEAR]->SetHiddenInGame(false);
		HairEffectComponent->SetSkeletalMesh(nullptr);
		HairEffectComponent->SetHiddenInGame(true);
		HairEffectComponent->SetActive(false);
		return -1;
	}

	int32 SlotIdx = GetSlotIndex(Comp);
	if (SlotIdx < 0)
		return -1;

	// 메시가 비었으면 슬롯 초기화
	if (SlotComponents[SlotIdx]->GetSkeletalMesh() == nullptr)
	{
		SlotComponents[SlotIdx]->SetOriginalSkeletalMesh(nullptr);
		SlotComponents[SlotIdx]->ResetMaterialData();
		SlotComponents[SlotIdx]->ResetSlicerData();

		const FPJItemData* ResolvedItemData = UPJDataTables::Resolve<UPJItemDataTable>(ItemID);
		if (ResolvedItemData == nullptr)
			ItemID = 0;
		EquipCostumeData.setDataTID((EPJEquipSlotTypeEnum)SlotIdx, ItemID, (int32)GenderType, 0);
	}

	// HEADWEAR 이상 장비 슬롯 처리
	if ((int32)EPJEquipSlotTypeEnum::HEADWEAR <= SlotIdx && SlotIdx < (int32)EPJEquipSlotTypeEnum::MAXCOUNT)
	{
		EPJEquipSlotTypeEnum EquipSlot = (EPJEquipSlotTypeEnum)SlotIdx;
		if (ItemID > 0)
			EquipCostumeData.setDataTID(EquipSlot, ItemID, (int32)GenderType, 0);

		FPJItemData* EquipItemData = UPJDataTables::Resolve<UPJItemDataTable>(EquipCostumeData.GetDataTID(EquipSlot));
		if (EquipItemData)
		{
			GetSlotComponent(EquipSlot)->ResetSlicerData();
			SetSliceInfo(EquipSlot, EquipItemData->Name);
		}
	}

	if (LODSyncComponent)
		LODSyncComponent->RefreshSyncComponents();

	OnSlotLoadCompleted.Broadcast((EPJEquipSlotTypeEnum)SlotIdx, ItemID);

	return SlotIdx;
}

void UPJCostumeEquipComponent::HandleAnimLoadCompleted(UPJSkeletalMeshComponent* Comp, USkeletalMeshComponent* RootMesh)
{
	// Body anim 완료는 Character 레벨에서 처리 (AnimInstance 초기화 등)
	// 이 컴포넌트에서는 슬롯 관련 후처리만 담당
	if (Comp == RootMesh)
		return;

	// 슬롯 컴포넌트의 애니메이션 로딩 완료 시 LOD 동기화
	if (LODSyncComponent)
		LODSyncComponent->RefreshSyncComponents();
}

void UPJCostumeEquipComponent::HandleMaterialLoadComplete(UPJSkeletalMeshComponent* Comp)
{
	int32 SlotIdx = GetSlotIndex(Comp);
	if (SlotIdx < 0)
		return;

	if (SlotIdx == (int32)EPJEquipSlotTypeEnum::BODY)
	{
		SlotComponents[SlotIdx]->ResetMaterialData(false);
	}
}


// ═══════════════════════════════════════════════════════════════════════
//  의상 모프 조절
// ═══════════════════════════════════════════════════════════════════════

void UPJCostumeEquipComponent::CheckClothMorphReady(FPJCustomizePresetData& CustomizeDatas, EPJGenderType GenderType)
{
	if (SlotComponents[(int)EPJEquipSlotTypeEnum::BODY]->GetOriginalSkeletalMesh() == nullptr)
		return;

	// 모든 슬롯의 로딩이 완료되었는지 확인
	for (int32 EnumIndex = (int32)EPJEquipSlotTypeEnum::HEADWEAR; EnumIndex < (int32)EPJEquipSlotTypeEnum::MAXCOUNT; ++EnumIndex)
	{
		if (!EquipCostumeData.IsSameSlot(LoadCostumeData, (EPJEquipSlotTypeEnum)EnumIndex))
			return;
	}

	UpdateClothMorph(CustomizeDatas, GenderType);
}

void UPJCostumeEquipComponent::UpdateClothMorph(FPJCustomizePresetData& CustomizeDatas, EPJGenderType GenderType)
{
	FPJBasicStyleData* StyleData = UPJDataTables::Resolve<UPJBasicStyleDataTable>(CustomizeDatas.BasicStyleDataID);
	ensure(StyleData);
	if (!StyleData)
		return;

	// 슬롯별 메시 활성화/비활성화
	for (int32 i = 0; i < (int32)SlotComponents.Num(); i++)
	{
		if (SlotComponents[i] != nullptr && SlotComponents[i]->GetOriginalSkeletalMesh() != nullptr)
		{
			TArray<TSharedPtr<FSliceConditionBase>> targetlist;
			SlotComponents[i]->SetActive(true);
			if (i != (int)EPJEquipSlotTypeEnum::HEAD)
				SlotComponents[i]->MakeCloneSkeletalMesh(targetlist, nullptr);
		}
		else if (SlotComponents[i] != nullptr && SlotComponents[i]->GetOriginalSkeletalMesh() == nullptr)
		{
			SlotComponents[i]->SetActive(false);
		}
	}

	// 의상 조합에 따른 바디 모프 데이터 계산 및 적용
	FPJClothMorphData MorphData = FPJClothMorphData::CalcClothMorphData(EquipCostumeData);
	FSoftObjectPath PathName = StyleData->MTU3;
	EPJCustomizeUnderWearType NewWearType = FPJClothMorphData::GetUnderWearType(StyleData, MorphData, PathName);

	if (PathName.IsValid() && NewWearType != CustomizeDatas.UnderWearType)
	{
		CustomizeDatas.UnderWearType = NewWearType;
		LoadMaterialSocket(PathName, EPJEquipSlotTypeEnum::BODY, TEXT("body"));
	}

	FPJClothMorphData::UpdateMorphData(SlotComponents, MorphData);

	// 헤어 타입 갱신 (모자 등에 의한 변형)
	EPJHairType HairType = FPJClothMorphData::GetUpdateHairType(EquipCostumeData, GenderType);

	if (LODSyncComponent)
		LODSyncComponent->RefreshSyncComponents();
}

void UPJCostumeEquipComponent::SetSliceInfo(EPJEquipSlotTypeEnum SocketType, FString Name)
{
	UPJSkeletalMeshComponent* SkelComp = GetSlotComponent(SocketType);
	if (SkelComp)
		SkelComp->SetSlicerData(Name);
}


// ═══════════════════════════════════════════════════════════════════════
//  Slot Access
// ═══════════════════════════════════════════════════════════════════════

UPJSkeletalMeshComponent* UPJCostumeEquipComponent::GetSlotComponent(EPJEquipSlotTypeEnum Part)
{
	if (SlotComponents.IsEmpty())
		return nullptr;
	if (Part == EPJEquipSlotTypeEnum::MAXCOUNT)
		return nullptr;
	return SlotComponents[(int32)Part];
}

int32 UPJCostumeEquipComponent::GetSlotIndex(UPJSkeletalMeshComponent* Comp)
{
	for (int32 i = 0; i < (int32)SlotComponents.Num(); ++i)
	{
		if (SlotComponents[i] == Comp)
			return i;
	}
	return -1;
}

void UPJCostumeEquipComponent::SetLOD(int32 Level)
{
	if (LODSyncComponent)
	{
		LODSyncComponent->ForcedLOD = Level;
		LODSyncComponent->RefreshSyncComponents();
	}
}
