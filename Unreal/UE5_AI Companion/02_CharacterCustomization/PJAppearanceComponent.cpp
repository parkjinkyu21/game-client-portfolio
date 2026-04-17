// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJAppearanceComponent.h"
#include "PJCostumeEquipComponent.h"
#include "PJSkeletalMeshComponent.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Initialize
// ═══════════════════════════════════════════════════════════════════════════════

void UPJAppearanceComponent::Initialize(UPJCostumeEquipComponent* InEquipComponent)
{
	EquipComponent = InEquipComponent;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Body Shape (체형)
// ═══════════════════════════════════════════════════════════════════════════════

void UPJAppearanceComponent::UpdateBodyShape()
{
	for (auto& MaterialData : BodyShape.MaterialDatas)
	{
		SetMaterialData(EPJEquipSlotTypeEnum::BODY, MaterialData, true);
	}
}

void UPJAppearanceComponent::SetBodyShape(FPJCustomizedBodyShape& NewShape)
{
	BodyShape = NewShape;
	UpdateBodyShape();
}

void UPJAppearanceComponent::UpdateNewBodyShape(int32 ID, FPJCustomizePresetData& CustomizeDatas)
{
	CustomizeDatas.BodyShapeDataID = ID;
	FPJBodyShapeData* BodyShapeData = UPJDataTables::Resolve<UPJBodyShapeDataTableExtension>(CustomizeDatas.BodyShapeDataID);
	if (BodyShapeData)
	{
		FPJCustomizedBodyShape* BodyShape = UPJDataTables::Resolve<UPJBodyShapeDataTableExtension>()->GetCustomizeData(BodyShapeData->ShapeFileName);
		if (BodyShape)
			SetBodyShape(*BodyShape);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// Morph Targets (모프 타겟)
// ═══════════════════════════════════════════════════════════════════════════════

void UPJAppearanceComponent::UpdateMorphDatas(FPJCustomizePresetData& Data)
{
	ClearMorphData();
	auto settings = UPJDataTables::ToKeyValueArray<UPJMorphSetDataTable>();
	if (settings.Num() > Data.MorpthTargetDatas.Num())
		Data.MorpthTargetDatas.Init(0, settings.Num());

	int32 Size = FMath::Min(settings.Num(), Data.MorpthTargetDatas.Num());
	for (int32 i = 0; i < Size; ++i)
	{
		int32 Index = (int32)settings[i].Value->Type;
		SetMorphData(*FString::FromInt(settings[i].Key), Data.MorpthTargetDatas[Index]);
	}
}

void UPJAppearanceComponent::SetMorphData(FName MorphName, float Value)
{
	AddCustomMorphData(MorphName, Value);
	EquipComponent->GetSlotComponent(EPJEquipSlotTypeEnum::HEAD)->SetMorphTarget(MorphName, Value);
}

void UPJAppearanceComponent::ClearMorphData()
{
	MorphDataArray.Empty();
	UPJSkeletalMeshComponent* head = EquipComponent->GetSlotComponent(EPJEquipSlotTypeEnum::HEAD);
	if (head)
		head->ClearMorphTargets();
}

void UPJAppearanceComponent::ResetMorphDatas(FPJCustomizePresetData& CustomizeDatas)
{
	ClearMorphData();
	UpdateMorphDatas(CustomizeDatas);
	UpdateMaterialDatas(CustomizeDatas);
}

void UPJAppearanceComponent::AddCustomMorphData(FName Name, float Value)
{
	for (int32 i = 0; i < MorphDataArray.Num(); ++i)
	{
		if (MorphDataArray[i].MorphName == Name)
		{
			MorphDataArray[i].Value = Value;
			return;
		}
	}
	FPJMoprhDatas MorphData;
	MorphData.MorphName = Name;
	MorphData.Value = Value;
	MorphDataArray.Add(MorphData);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Materials (머티리얼 파라미터)
// ═══════════════════════════════════════════════════════════════════════════════

void UPJAppearanceComponent::UpdateMaterialDatas(FPJCustomizePresetData& Data)
{
	// 눈썹 색상/강도
	FPJMaterialDatas MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_COLOR, TEXT("Eyebrow_Color"), Data.EyeblowColor);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);

	MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_FLOAT, TEXT("EyeblowPower"), Data.EyeblowPower);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);

	// 눈썹 형태
	FPJEyebrowshape* EyebrowShapeData = UPJDataTables::Resolve<UPJEyebrowshapeTable>(Data.EyeblowShape);
	if (EyebrowShapeData)
	{
		MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_TEXTURE, TEXT("Haircap_Eyebrow_Mask"), EyebrowShapeData->MaskTexture);
		SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, true);
		MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Eyebrow_Index"), EyebrowShapeData->Index);
		SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, true);
	}

	// 홍채 색상/밝기/동공 크기
	MatData = FPJMaterialDatas(TEXT("eye"), EPJMaterialParamType::PJMP_COLOR, TEXT("IrisColor"), Data.IrisColor);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);

	MatData = FPJMaterialDatas(TEXT("eye"), EPJMaterialParamType::PJMP_FLOAT, TEXT("IrisBrightness"), Data.IrisPower);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);

	MatData = FPJMaterialDatas(TEXT("eye"), EPJMaterialParamType::PJMP_FLOAT, TEXT("PupilScale"), Data.PupilScale);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);
}

void UPJAppearanceComponent::SetMaterialData(EPJEquipSlotTypeEnum Socket, FPJMaterialDatas& Data, bool bApplyTexture)
{
	if (Socket != EPJEquipSlotTypeEnum::MAXCOUNT)
	{
		UPJSkeletalMeshComponent* SlotComp = EquipComponent->GetSlotComponent(Socket);
		if (SlotComp)
			SlotComp->ChangeMaterialData(Data, bApplyTexture);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// Hair (헤어)
// ═══════════════════════════════════════════════════════════════════════════════

void UPJAppearanceComponent::UpdateHairColor(FPJCustomizePresetData& CustomizeDatas, bool bApplyTexture)
{
	UPJSkeletalMeshComponent* HairComp = EquipComponent->GetSlotComponent(EPJEquipSlotTypeEnum::HAIR);
	if (HairComp)
		HairComp->ResetMaterialData(true);

	// Haircap 색상
	FPJMaterialDatas MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_COLOR, TEXT("Haircap_Color"), CustomizeDatas.HairRootColor);
	SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);

	// 헤어 색상 파라미터
	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_COLOR, TEXT("Root_Color"), CustomizeDatas.HairRootColor);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_COLOR, TEXT("Tip_Color"), CustomizeDatas.HairTipColor);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Scatter"), CustomizeDatas.HairBrightness);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Gloss"), CustomizeDatas.HairRoughness);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Offset"), CustomizeDatas.HairColorRange);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	MatData = FPJMaterialDatas(TEXT("hair"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Spread"), CustomizeDatas.HairColorContrast);
	SetMaterialData(EPJEquipSlotTypeEnum::HAIR, MatData, false);

	// 헤어캡 마스크 텍스처
	if (FPJHairMeshData* HairMeshData = UPJDataTables::Resolve<UPJHairMeshDataTable>(CustomizeDatas.HairMeshDataID))
	{
		MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_TEXTURE, TEXT("Haircap_Eyebrow_Mask"), HairMeshData->MaskTexture);
		SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, bApplyTexture);
		MatData = FPJMaterialDatas(TEXT("face"), EPJMaterialParamType::PJMP_FLOAT, TEXT("Haircap_Index"), HairMeshData->Index);
		SetMaterialData(EPJEquipSlotTypeEnum::HEAD, MatData, false);
	}
}

void UPJAppearanceComponent::SetHairMesh(int32 ID, EPJHairType type, FPJCustomizePresetData& CustomizeDatas)
{
	if (CustomizeDatas.HairMeshDataID == ID && CustomizeDatas.HairMeshSubType == type)
		return;

	CustomizeDatas.HairMeshDataID = ID;
	CustomizeDatas.HairMeshSubType = type;
	if (FPJHairMeshData* HairMeshData = UPJDataTables::Resolve<UPJHairMeshDataTable>(CustomizeDatas.HairMeshDataID))
	{
		switch (type)
		{
		case EPJHairType::NORMAL:
		default:
			EquipComponent->LoadSkeletalSocket(HairMeshData->MeshAsset, EPJEquipSlotTypeEnum::HAIR);
			break;
		case EPJHairType::CAP:
			EquipComponent->LoadSkeletalSocket(HairMeshData->CAP, EPJEquipSlotTypeEnum::HAIR);
			break;
		case EPJHairType::HAT:
			EquipComponent->LoadSkeletalSocket(HairMeshData->HAT, EPJEquipSlotTypeEnum::HAIR);
			break;
		case EPJHairType::HELMET:
			EquipComponent->LoadSkeletalSocket(HairMeshData->HELMET, EPJEquipSlotTypeEnum::HAIR);
			break;
		case EPJHairType::SUNCAP:
			EquipComponent->LoadSkeletalSocket(HairMeshData->SUNCAP, EPJEquipSlotTypeEnum::HAIR);
			break;
		}
	}
}
