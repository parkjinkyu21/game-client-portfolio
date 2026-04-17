// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PJCostumeInterface.h"
#include "PJCostumeCharacter.generated.h"

class UPJCostumeEquipComponent;
class UPJAppearanceComponent;
class UPJShapeAnimInstance;
struct FPJBasicStyleData;

/**
 * 아바타 커스터마이징 캐릭터 (Mediator)
 *
 * 코스튬 장비(CostumeEquipComponent)와 외형 커스터마이징(AppearanceComponent)을
 * 소유하고 두 컴포넌트 간의 협력을 조율하는 오케스트레이터.
 *
 * [컴포넌트 구조]
 * APJCostumeCharacter (Mediator)
 *   ├── UPJCostumeEquipComponent : 슬롯 기반 메시 장착, 커팅, LOD
 *   └── UPJAppearanceComponent   : 체형, 모프, 머티리얼, 헤어
 *
 * [조율 흐름]
 * 1. SetCustomizeDatas() → 두 컴포넌트에 초기 데이터 분배
 * 2. CostumeEquipComponent가 메시 비동기 로딩 완료
 * 3. IPJCostumeInterface 콜백 → Character가 수신
 * 4. 슬롯 타입에 따라 AppearanceComponent에 외형 업데이트 트리거
 */
UCLASS(Blueprintable)
class PJCORE_API APJCostumeCharacter : public ACharacter, public IPJCostumeInterface
{
	GENERATED_BODY()

public:
	APJCostumeCharacter(const FObjectInitializer& ObjectInitializer);

// ─── Lifecycle ──────────────────────────────────────────────────────
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

// ─── Orchestration (데이터 분배 및 조율) ────────────────────────────
public:
	void SetCustomizeDatas(const FPJCustomizePresetData& Data);
	void SetStyleData(int32 ID, FPJBasicStyleData* StyleData);
	void SetEquipCostumeData(FPJEquipCostumeData& Data);

	FPJCustomizePresetData& GetCustomizeData() { return CustomizeDatas; }

// ─── Component Access ───────────────────────────────────────────────
public:
	UPJCostumeEquipComponent* GetCostumeEquipComponent() const { return CostumeEquipComponent; }
	UPJAppearanceComponent*   GetAppearanceComponent() const { return AppearanceComponent; }

	EPJGenderType GetGenderType() const { return GenderType; }
	EPJRaceType   GetRaceType() const { return RaceType; }

// ─── IPJCostumeInterface (비동기 로딩 콜백 → 컴포넌트 디스패치) ─────
protected:
	virtual void OnSkeletalLoadCompleted(class UPJSkeletalMeshComponent* Comp, int32 ItemID) override;
	virtual void OnAnimLoadCompleted(class UPJSkeletalMeshComponent* Comp) override;
	virtual void OnPreLoadSkeletalMesh(class UPJSkeletalMeshComponent* Comp) override {}
	virtual void OnStaticLoadCompleted(class UPJStaticMeshComponent* Comp) override {}
	virtual void OnPreLoadStaticMesh(class UPJStaticMeshComponent* Comp) override {}
	virtual void OnMaterialLoadComplete(class UPJSkeletalMeshComponent* Comp) override;

// ─── Components ─────────────────────────────────────────────────────
protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "PJComponents")
	TObjectPtr<UPJCostumeEquipComponent> CostumeEquipComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "PJComponents")
	TObjectPtr<UPJAppearanceComponent> AppearanceComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PJComponents", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULODSyncComponent> LODSyncComponent;

// ─── Data ───────────────────────────────────────────────────────────
protected:
	FPJCustomizePresetData CustomizeDatas;
	EPJGenderType GenderType;
	EPJRaceType RaceType;
};
