// AI Character Companion Application
// Built with Unreal Engine 5
﻿#pragma once
#include "CoreMinimal.h"
#include "PJClothMorphData.generated.h"

struct FPJItemData;
struct FPJClothItemData;
struct FPJEquipCostumeData;
enum class EPJGenderType : uint8;

/**
 * 의상 조합 기반 바디 모프 데이터
 *
 * 상의/하의/신발/양말/아우터 조합에 따라 바디 메시의 모프 타겟 값을 자동 계산.
 * 예: 반바지 착용 시 BodyShortPants 활성화, 롱부츠 착용 시 BottomAnkleShrink 적용
 * CalcClothMorphData()에서 데이터 테이블 기반으로 모프 값을 합산/우선순위 처리.
 */
USTRUCT(BlueprintType)
struct FPJClothMorphData
{
	GENERATED_BODY()

public:
	// 속옷
	bool UnderWear_Top = true;
	bool UnderWear_Bottom = true;


	// Outter
	UPROPERTY()
	float BodyAll = 0.f;
	UPROPERTY()
	float BodyTorso = 0.f;
	UPROPERTY()
	float BodyTorso2 = 0.f;
	UPROPERTY()
	float BodyOuter = 0.f;
	UPROPERTY()
	float BodyOuterVest = 0.f;
	UPROPERTY()
	float TopAll = 0.f;
	UPROPERTY()
	float TopTorso = 0.f;
	UPROPERTY()
	float TopTorso2 = 0.f;	

	// Top
	UPROPERTY()
	float BodyNoBra = 0.f;
	UPROPERTY()
	float BodyLongSleeve = 0.f;
	UPROPERTY()
	float BodyMiddleSleeve = 0.f;
	UPROPERTY()
	float BodyShortSleeve = 0.f;
	UPROPERTY()
	float BodySleeveless = 0.f;
	UPROPERTY()
	float BodyBeltShrink = 0.f;
	UPROPERTY()
	float SocksHipShrink = 0.f;
	UPROPERTY()
	float SocksBeltShrink = 0.f;
	UPROPERTY()
	float BottomHipShrink = 0.f;
	UPROPERTY()
	float BottomBeltShrink = 0.f;

	// Bottom 
	UPROPERTY()
	float BodyLongPants = 0.f;
	UPROPERTY()
	float BodyMiddlePants = 0.f;
	UPROPERTY()
	float BodyShortPants = 0.f;
	UPROPERTY()
	float BodySocksShrink = 0.f;
	UPROPERTY()
	float SocksShortPants = 0.f;

	// Shoes
	UPROPERTY()
	float BodyFootShrink = 0.f;
	UPROPERTY()
	float SocksFootShrink = 0.f;
	UPROPERTY()
	float BottomAnkleShrink = 0.f;
	UPROPERTY()
	float BottomAnkleGrow = 0.f;
	UPROPERTY()
	float BottomSocksShrink = 0.f;
	
public:
	static FPJClothMorphData	CalcClothMorphData(const	FPJEquipCostumeData& EquipCostumeData);
	static void					UpdateMorphData(TArray<TObjectPtr<class UPJSkeletalMeshComponent> >& MeshList, const FPJClothMorphData& MorphData);
	static EPJCustomizeUnderWearType GetUnderWearType(struct FPJBasicStyleData* StyleData, const FPJClothMorphData& MorphData, FSoftObjectPath& PathName);

	static EPJHairType	GetUpdateHairType(const	FPJEquipCostumeData& EquipCostumeData, EPJGenderType GenderType);

};

