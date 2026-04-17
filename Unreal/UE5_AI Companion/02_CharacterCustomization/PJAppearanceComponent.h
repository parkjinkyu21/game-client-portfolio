// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PJAppearanceComponent.generated.h"

class UPJSkeletalMeshComponent;
class UPJCostumeEquipComponent;
struct FPJMaterialDatas;
enum class EPJMaterialParamType : uint8;

/**
 * 외형 커스터마이징 컴포넌트
 *
 * 캐릭터의 시각적 외형을 관리한다.
 * CostumeEquipComponent가 제공하는 메시 슬롯에 모프 타겟, 머티리얼 파라미터,
 * 헤어 색상 등을 적용하여 아바타의 개성을 표현한다.
 *
 * [핵심 기능]
 * - 모프 타겟 기반 체형 변경: 데이터 테이블 기반 얼굴/신체 형태 조절
 * - 머티리얼 인스턴스 제어: 슬롯별 Color/Float/Texture 파라미터 동적 변경
 * - 헤어 시스템: 색상 그라디언트(Root/Tip), 광택, 거칠기 등 실시간 조절
 * - 눈썹/홍채: 형태 마스크, 색상, 밝기 개별 제어
 */
UCLASS()
class PJCORE_API UPJAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void Initialize(UPJCostumeEquipComponent* InEquipComponent);

// ─── Body Shape (체형) ──────────────────────────────────────────────
public:
	void SetBodyShape(FPJCustomizedBodyShape& NewShape);
	void UpdateBodyShape();
	void UpdateNewBodyShape(int32 ID, FPJCustomizePresetData& CustomizeDatas);
	FPJCustomizedBodyShape& GetBodyShape() { return BodyShape; }

// ─── Morph Targets (모프 타겟) ──────────────────────────────────────
public:
	void UpdateMorphDatas(FPJCustomizePresetData& Data);
	void SetMorphData(FName MorphName, float Value);
	void ClearMorphData();
	void ResetMorphDatas(FPJCustomizePresetData& CustomizeDatas);

// ─── Materials (머티리얼 파라미터) ──────────────────────────────────
public:
	void UpdateMaterialDatas(FPJCustomizePresetData& Data);
	void SetMaterialData(EPJEquipSlotTypeEnum Socket, FPJMaterialDatas& Data, bool bApplyTexture = true);

// ─── Hair (헤어) ────────────────────────────────────────────────────
public:
	void UpdateHairColor(FPJCustomizePresetData& CustomizeDatas, bool bApplyTexture = true);
	void SetHairMesh(int32 ID, EPJHairType type, FPJCustomizePresetData& CustomizeDatas);

private:
	void AddCustomMorphData(FName Name, float Value);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPJCostumeEquipComponent> EquipComponent;

	FPJCustomizedBodyShape BodyShape;
	TArray<FPJMoprhDatas> MorphDataArray;
};
