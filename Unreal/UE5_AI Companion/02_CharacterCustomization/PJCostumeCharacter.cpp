// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJCostumeCharacter.h"


// ═══════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════

APJCostumeCharacter::APJCostumeCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPJSkeletalMeshComponent>(MeshComponentName))
{
	GetMesh()->SetCollisionProfileName(TEXT("CharacterCollision"));
	GetMesh()->SetVisibility(false);
	GetMesh()->SetHiddenInGame(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	LODSyncComponent = CreateDefaultSubobject<ULODSyncComponent>(TEXT("LodSync"));
	LODSyncComponent->ComponentsToSync.Emplace(MeshComponentName, ESyncOption::Passive);

	CostumeEquipComponent = CreateDefaultSubobject<UPJCostumeEquipComponent>(TEXT("CostumeEquip"));
	AppearanceComponent = CreateDefaultSubobject<UPJAppearanceComponent>(TEXT("Appearance"));
}

void APJCostumeCharacter::BeginPlay()
{
	Super::BeginPlay();

	CostumeEquipComponent->InitializeSlots(GetMesh());
	AppearanceComponent->Initialize(CostumeEquipComponent);
}

void APJCostumeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CostumeEquipComponent->RemoveAllCostume();

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		movement->GravityScale = 1.0f;
		movement->SetMovementMode(MOVE_Walking);
	}

	CostumeEquipComponent->CleanupSlots();

	Super::EndPlay(EndPlayReason);
}

// ═══════════════════════════════════════════════════════════════════════
//  Orchestration - 두 컴포넌트 간 데이터 분배 및 조율
// ═══════════════════════════════════════════════════════════════════════

void APJCostumeCharacter::SetCustomizeDatas(const FPJCustomizePresetData& Data)
{
	CostumeEquipComponent->RemoveAll();
	CostumeEquipComponent->ResetCostumeData();
	CustomizeDatas = Data;

	if (CustomizeDatas.BasicStyleDataID == 0)
		CustomizeDatas.BasicStyleDataID = 10001;

	FPJBasicStyleData* StyleData = UPJDataTables::Resolve<UPJBasicStyleDataTable>(CustomizeDatas.BasicStyleDataID);
	SetStyleData(CustomizeDatas.BasicStyleDataID, StyleData);

	// Body Shape 적용
	FPJBodyShapeData* BodyShapeData = UPJDataTables::Resolve<UPJBodyShapeDataTableExtension>(CustomizeDatas.BodyShapeDataID);
	if (BodyShapeData)
	{
		FPJCustomizedBodyShape* BodyShapeCustom = UPJDataTables::Resolve<UPJBodyShapeDataTableExtension>()->GetCustomizeData(BodyShapeData->ShapeFileName);
		if (BodyShapeCustom)
		{
			AppearanceComponent->SetBodyShape(*BodyShapeCustom);
		}
	}

	int32 HairID = CustomizeDatas.HairMeshDataID;
	CustomizeDatas.HairMeshDataID = 0;
	AppearanceComponent->SetHairMesh(HairID, EPJHairType::NORMAL, CustomizeDatas);
}

void APJCostumeCharacter::SetStyleData(int32 ID, FPJBasicStyleData* StyleData)
{
	if (StyleData == nullptr)
		return;

	if (GenderType != StyleData->Gender)
	{
		CostumeEquipComponent->RemoveAllCostume();
	}
	CustomizeDatas.BasicStyleDataID = ID;
	RaceType = StyleData->Race;
	GenderType = StyleData->Gender;

	// 성별에 따른 AnimBlueprint 로딩
	if (UPJSkeletalMeshComponent* BodyMesh = Cast<UPJSkeletalMeshComponent>(GetMesh()))
	{
		if (GenderType == EPJGenderType::MALE)
			BodyMesh->AsyncLoadMeshAnim(StyleData->SkeletalMeshAsset, FSoftClassPath(TEXT("/Script/Engine.AnimBlueprint'/Game/CommonAssets/BluePrints/Animations/BP_PJ_Anim_M.BP_PJ_Anim_M_C'")));
		else
			BodyMesh->AsyncLoadMeshAnim(StyleData->SkeletalMeshAsset, FSoftClassPath(TEXT("/Script/Engine.AnimBlueprint'/Game/CommonAssets/BluePrints/Animations/BP_PJ_Anim_F.BP_PJ_Anim_F_C'")));
	}

	CostumeEquipComponent->LoadSkeletalSocket(StyleData->HeadAsset, EPJEquipSlotTypeEnum::HEAD);
	CostumeEquipComponent->LoadSkeletalMaterialSocket(StyleData->BodyAsset, StyleData->MTU3, EPJEquipSlotTypeEnum::BODY, TEXT("body"));
}

void APJCostumeCharacter::SetEquipCostumeData(FPJEquipCostumeData& Data)
{
	CostumeEquipComponent->SetEquipCostumeData(Data, GenderType);
}


// ═══════════════════════════════════════════════════════════════════════
//  IPJCostumeInterface - 로딩 완료 콜백 디스패치
// ═══════════════════════════════════════════════════════════════════════

void APJCostumeCharacter::OnSkeletalLoadCompleted(UPJSkeletalMeshComponent* Comp, int32 ItemID)
{
	int32 SlotIndex = CostumeEquipComponent->HandleSkeletalLoadCompleted(Comp, GetMesh(), ItemID, GenderType);
	if (SlotIndex < 0)
		return;

	// 슬롯 타입에 따라 AppearanceComponent에 외형 업데이트 트리거
	if (SlotIndex == (int32)EPJEquipSlotTypeEnum::BODY)
	{
		AppearanceComponent->UpdateBodyShape();
		CostumeEquipComponent->SetSliceInfo(EPJEquipSlotTypeEnum::BODY, TEXT("body"));
	}
	if (SlotIndex == (int32)EPJEquipSlotTypeEnum::HEAD)
	{
		AppearanceComponent->ResetMorphDatas(CustomizeDatas);
	}
	if (SlotIndex == (int32)EPJEquipSlotTypeEnum::HAIR)
	{
		AppearanceComponent->UpdateHairColor(CustomizeDatas);
		CostumeEquipComponent->SetSliceInfo(EPJEquipSlotTypeEnum::HAIR, TEXT("hair"));
	}

	CostumeEquipComponent->CheckClothMorphReady(CustomizeDatas, GenderType);
}

void APJCostumeCharacter::OnAnimLoadCompleted(UPJSkeletalMeshComponent* Comp)
{
	CostumeEquipComponent->HandleAnimLoadCompleted(Comp, GetMesh());

	if (Comp == GetMesh())
	{
		AppearanceComponent->UpdateBodyShape();
	}
}

void APJCostumeCharacter::OnMaterialLoadComplete(UPJSkeletalMeshComponent* Comp)
{
	CostumeEquipComponent->HandleMaterialLoadComplete(Comp);

	int32 Index = CostumeEquipComponent->GetSlotIndex(Comp);
	if (Index == (int32)EPJEquipSlotTypeEnum::BODY)
	{
		AppearanceComponent->UpdateBodyShape();
	}
}
