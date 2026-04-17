// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PJCostumeEquipComponent.generated.h"

class UPJSkeletalMeshComponent;
class ULODSyncComponent;
struct FPJItemData;
struct FPJClothItemData;
struct FPJBasicStyleData;

DECLARE_LOG_CATEGORY_EXTERN(PJCostumeEquipComponent, Log, All);

// 슬롯 로딩 완료 시 브로드캐스트 (Character가 수신하여 Appearance 업데이트 트리거)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCostumeSlotLoaded, EPJEquipSlotTypeEnum /*Slot*/, int32 /*ItemID*/);

/**
 * 코스튬 장비 컴포넌트
 *
 * 부위별 스켈레탈 메시 슬롯을 관리하고, 비동기 로딩 및
 * 의상 조합에 따른 모프 조절을 처리한다.
 *
 * [핵심 기능]
 * - EPJEquipSlotTypeEnum 기반 소켓 메시 배열 관리
 * - 비동기 에셋 로딩 (StreamableHandle) 및 완료 콜백
 * - 의상 모프 데이터 계산 후 바디 메시 변형 (FPJClothMorphData)
 * - LOD 동기화: 장착된 메시 간 자동 Level-of-Detail 관리
 */
UCLASS()
class PJCORE_API UPJCostumeEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	FOnCostumeSlotLoaded OnSlotLoadCompleted;

// ─── Initialization ─────────────────────────────────────────────────
public:
	void InitializeSlots(USkeletalMeshComponent* RootMesh);
	void CleanupSlots();

// ─── Mesh Loading ───────────────────────────────────────────────────
public:
	void LoadSkeletalSocket(FSoftObjectPath ToLoad, EPJEquipSlotTypeEnum Part, int32 ItemID = -1);
	void LoadMaterialSocket(FSoftObjectPath ToLoad, EPJEquipSlotTypeEnum Part, FString SlotName);
	void LoadSkeletalMaterialSocket(FSoftObjectPath ToLoad, FSoftObjectPath ToMat, EPJEquipSlotTypeEnum Part, FString SlotName, int32 ItemID = -1);
	void LoadSkeletalAnimSocket(FSoftObjectPath ToLoad, FSoftObjectPath ToAnim, EPJEquipSlotTypeEnum Part);
	void SetSkeletalSocket(USkeletalMesh* Mesh, EPJEquipSlotTypeEnum Part);
	void RemoveSkeletalSocket(EPJEquipSlotTypeEnum Part, bool bForce = false);

// ─── Costume Data ───────────────────────────────────────────────────
public:
	void SetEquipCostumeData(FPJEquipCostumeData& Data, EPJGenderType GenderType);
	void LoadEquipCostumeData(EPJEquipSlotTypeEnum SocketType, int32 CostumeID, EPJGenderType GenderType);
	void SetEquipCostume(int32 CostumeID, EPJGenderType GenderType);
	void ReEquipCostumeData(EPJGenderType GenderType);
	void RemoveAll();
	void RemoveAllCostume();
	void ResetCostumeData();

	FPJEquipCostumeData& GetEquipCostumeData() { return EquipCostumeData; }
	FPJEquipCostumeData& GetLoadCostumeData() { return LoadCostumeData; }
	bool IsEquipItem(EPJEquipSlotTypeEnum Part, EPJItemSmallTypeEnum TypeSmall);

// ─── Slot Access ────────────────────────────────────────────────────
public:
	UPJSkeletalMeshComponent* GetSlotComponent(EPJEquipSlotTypeEnum Part);
	UPJSkeletalMeshComponent* GetHairEffectComponent() { return HairEffectComponent; }
	TArray<TObjectPtr<UPJSkeletalMeshComponent>>& GetSlotComponents() { return SlotComponents; }
	int32 GetSlotIndex(UPJSkeletalMeshComponent* Comp);
	void SetLOD(int32 Level);

// ─── 의상 모프 조절 ──────────────────────────────────────────────────
public:
	void UpdateClothMorph(FPJCustomizePresetData& CustomizeDatas, EPJGenderType GenderType);
	void SetSliceInfo(EPJEquipSlotTypeEnum SocketType, FString Name);

// ─── Load Callbacks (called by Character via IPJCostumeInterface) ───
public:
	// Returns the slot index, or -1. Character uses this to decide what appearance updates to trigger.
	int32 HandleSkeletalLoadCompleted(UPJSkeletalMeshComponent* Comp, USkeletalMeshComponent* RootMesh, int32 ItemID, EPJGenderType GenderType);
	void HandleAnimLoadCompleted(UPJSkeletalMeshComponent* Comp, USkeletalMeshComponent* RootMesh);
	void HandleMaterialLoadComplete(UPJSkeletalMeshComponent* Comp);

private:
	void LoadFromClothData(EPJEquipSlotTypeEnum Socket, int32 ItemID, EPJGenderType GenderType);
	void CheckClothMorphReady(FPJCustomizePresetData& CustomizeDatas, EPJGenderType GenderType);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPJSkeletalMeshComponent>> SlotComponents;

	UPROPERTY(Transient)
	TObjectPtr<UPJSkeletalMeshComponent> HairEffectComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULODSyncComponent> LODSyncComponent;

	FPJEquipCostumeData EquipCostumeData;
	FPJEquipCostumeData LoadCostumeData;
};
