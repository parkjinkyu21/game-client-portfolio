// AI Character Companion Application
// Built with Unreal Engine 5
﻿#include "PJClothMorphData.h"


FPJClothMorphData FPJClothMorphData::CalcClothMorphData(const FPJEquipCostumeData& EquipCostumeData)
{
	FPJClothItemData* ShoesData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::SHOES));
	FPJClothItemData* BotData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::BOTTOMS));
	FPJClothItemData* SocksData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::SOCKS));
	FPJClothItemData* TopData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::TOPS));
	FPJClothItemData* OutData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::OUTER));

	FPJClothMorphData ResultMorphData;
	if (OutData)
	{
		ResultMorphData.BodyAll = OutData->BodyAll;
		ResultMorphData.BodyTorso = OutData->BodyTorso;
		ResultMorphData.BodyTorso2 = OutData->BodyTorso2;
		ResultMorphData.TopAll = OutData->TopAll;
		ResultMorphData.TopTorso = OutData->TopTorso;
		ResultMorphData.TopTorso2 = OutData->TopTorso2;
	}
	if (TopData)
	{
		ResultMorphData.BodyNoBra = TopData->BodyNoBra;
		ResultMorphData.BodyBeltShrink = TopData->BodyBeltShrink;
		ResultMorphData.SocksHipShrink = TopData->SocksHipShrink;
		ResultMorphData.SocksBeltShrink = TopData->SocksBeltShrink;
		ResultMorphData.BottomHipShrink = TopData->BottomHipShrink;
		ResultMorphData.BottomBeltShrink = TopData->BottomBeltShrink;

		// Top 과 Outer 동시 착용 시
		if (OutData)
		{
			// Top 과 Outer 동시 착용 시에는 BodyLongSleeve, BodyMiddleSleeve, BodyShortSleeve, BodySleeveless값은 0으로 설정된다.

			// 테이블의 BodySleeveless 값이  0보다 큰값이면 BodyTorso = 0 ,BodyTorso2 = 1
			if(TopData->BodySleeveless > 0.f)
			{
				ResultMorphData.BodyTorso = 0.f;
				ResultMorphData.BodyTorso2 = 1.f;
			}
			// 아니면 BodyTorso2 = 0으로 설정
			else
			{
				ResultMorphData.BodyTorso2 = 0.f;
			}
		}
		// Outer 없이 Top만 착용 시
		else
		{
			// Outer없이 Top만 착용시 BodyMorph값 BodyLongSleeve, BodyMiddleSleeve, BodyShortSleeve, BodySleeveless 값을 사용한다.
			ResultMorphData.BodyLongSleeve = TopData->BodyLongSleeve;
			ResultMorphData.BodyMiddleSleeve = TopData->BodyMiddleSleeve;
			ResultMorphData.BodyShortSleeve = TopData->BodyShortSleeve;
			ResultMorphData.BodySleeveless = TopData->BodySleeveless;
		}
		// 상의에 의해 속옷 처리
		ResultMorphData.UnderWear_Top = TopData->UnderclothTop;
	}
	// Top을 착용하지 않고 Outer만 착용하는경우
	else if (OutData)
	{
		// BodyAll, BodyTorso, BodyTorso2 의 값을 0으로 하고 BodyOuter, BodyOuterVest를 사용
		ResultMorphData.BodyAll = 0.f;
		ResultMorphData.BodyTorso = 0.f;
		ResultMorphData.BodyTorso2 = 0.f;
		ResultMorphData.BodyOuter = OutData->BodyOuter;
		ResultMorphData.BodyOuterVest = OutData->BodyOuterVest;
	}

	if (BotData)
	{
		ResultMorphData.BodyLongPants = BotData->BodyLongPants;
		ResultMorphData.BodyMiddlePants = BotData->BodyMiddlePants;
		ResultMorphData.BodyShortPants = BotData->BodyShortPants;
		ResultMorphData.BodySocksShrink = BotData->BodySocksShrink;
		ResultMorphData.SocksShortPants = BotData->SocksShortPants;

		// 하의에 의해 속옷 처리
		ResultMorphData.UnderWear_Bottom = BotData->UnderclothUnder;
	}
	if (SocksData)
	{
		// BodySocksShrink 중복발생시 큰수를 따라간다.
		ResultMorphData.BodySocksShrink = FMath::Max(ResultMorphData.BodySocksShrink, SocksData->BodySocksShrink);
		ResultMorphData.BodyFootShrink = SocksData->BodyFootShrink;
		ResultMorphData.BottomSocksShrink = SocksData->BottomSocksShrink;
	}
	if (ShoesData)
	{
		// BodyFootShrink 중복발생시 큰수를 따라간다.
		ResultMorphData.BodyFootShrink = FMath::Max(ResultMorphData.BodyFootShrink, ShoesData->BodyFootShrink);
		ResultMorphData.SocksFootShrink = ShoesData->SocksFootShrink;
		ResultMorphData.BottomAnkleShrink = ShoesData->BottomAnkleShrink;
		ResultMorphData.BottomAnkleGrow = ShoesData->BottomAnkleGrow;
	}

	return ResultMorphData;
}

void FPJClothMorphData::UpdateMorphData(TArray<TObjectPtr<UPJSkeletalMeshComponent> >& MeshList, const FPJClothMorphData& MorphData)
{
	MeshList[(int32)EPJEquipSlotTypeEnum::BODY]->SetClothMorphTarget(TEXT("Body"), MorphData);
	MeshList[(int32)EPJEquipSlotTypeEnum::TOPS]->SetClothMorphTarget(TEXT("Top"), MorphData);
	MeshList[(int32)EPJEquipSlotTypeEnum::BOTTOMS]->SetClothMorphTarget(TEXT("Bottom"), MorphData);
	MeshList[(int32)EPJEquipSlotTypeEnum::SOCKS]->SetClothMorphTarget(TEXT("Socks"), MorphData);
}

EPJCustomizeUnderWearType FPJClothMorphData::GetUnderWearType(struct FPJBasicStyleData* StyleData, const FPJClothMorphData& MorphData, FSoftObjectPath& PathName)
{
	EPJCustomizeUnderWearType NewWearType = EPJCustomizeUnderWearType::UNDERWEAR;
	PathName = StyleData->MTU3;
	if (MorphData.UnderWear_Top)	// 상의 보이기
	{
		if (MorphData.UnderWear_Bottom) // 상하의 보이기
		{
			PathName = StyleData->MTU3;
			NewWearType = EPJCustomizeUnderWearType::UNDERWEAR;
		}
		else // 상의만 보이기
		{
			PathName = StyleData->MTU1;
			NewWearType = EPJCustomizeUnderWearType::TOP;
		}
	}
	else
	{
		if (MorphData.UnderWear_Bottom) // 하의만 보이기
		{
			PathName = StyleData->MTU2;
			NewWearType = EPJCustomizeUnderWearType::BOTTOM;
		}
		else // 속옷 안보이기
		{
			PathName = StyleData->MTU0;
			NewWearType = EPJCustomizeUnderWearType::ALL_NAKED;
		}
	}

	return NewWearType;
}

EPJHairType FPJClothMorphData::GetUpdateHairType(const FPJEquipCostumeData& EquipCostumeData, EPJGenderType GenderType)
{
	EPJHairType ResultHairType = EPJHairType::NORMAL;
	if (EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::TOPS) != 0)
	{
		if (FPJClothItemData* ClothItemData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::TOPS)))
		{
			EPJHairType TempHairType = GenderType == EPJGenderType::MALE ? ClothItemData->HearSetM : ClothItemData->HearSetF;
			if (TempHairType != EPJHairType::NORMAL)
				ResultHairType = TempHairType;
		}
	}
	if (EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::OUTER) != 0)
	{
		if (FPJClothItemData* ClothItemData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::OUTER)))
		{
			EPJHairType TempHairType = GenderType == EPJGenderType::MALE ? ClothItemData->HearSetM : ClothItemData->HearSetF;
			if (TempHairType != EPJHairType::NORMAL)
				ResultHairType = TempHairType;
		}
	}
	if (EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::HEADWEAR) != 0)
	{
		if (FPJClothItemData* ClothItemData = UPJDataTables::Resolve<UPJClothItemDataTable>(EquipCostumeData.GetDataTID(EPJEquipSlotTypeEnum::HEADWEAR)))
		{
			EPJHairType TempHairType = GenderType == EPJGenderType::MALE ? ClothItemData->HearSetM : ClothItemData->HearSetF;
			if (TempHairType != EPJHairType::NORMAL)
				ResultHairType = TempHairType;
		}
	}
	return ResultHairType;
}
