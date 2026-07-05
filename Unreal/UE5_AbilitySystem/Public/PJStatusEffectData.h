#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PJAbilityTypes.h"
#include "PJStatusEffectData.generated.h"

/**
 * 상태효과 정의 데이터에셋(버프/디버프/도트).
 * 효과를 데이터로 정의한다. 런타임 인스턴스는 UPJStatusEffect.
 *
 * [지속 정책별 스탯 반영 방식]
 * - Instant   : Modifiers를 스탯에 즉시 1회 적용(Base/Current 변경).
 * - Duration  : 수명 동안 Modifiers를 스탯에 주입, 만료 시 소스 핸들로 회수. 시한 버프.
 * - Infinite  : 수동 제거 전까지 유지.
 * - Period>0  : 주기마다 PeriodStatTag에 PeriodMagnitude를 Instant 적용.
 */
UCLASS(BlueprintType)
class PJABILITYSYSTEM_API UPJStatusEffectData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) FGameplayTag EffectTag;
    UPROPERTY(EditDefaultsOnly) EPJEffectDuration DurationPolicy = EPJEffectDuration::Duration;
    UPROPERTY(EditDefaultsOnly) float Duration = 5.f;

    UPROPERTY(EditDefaultsOnly) float Period = 0.f;          // 0 = 비주기
    UPROPERTY(EditDefaultsOnly) FGameplayTag PeriodStatTag;  // 주기 적용 대상 스탯
    UPROPERTY(EditDefaultsOnly) float PeriodMagnitude = 0.f; // 주기당 Instant 델타

    UPROPERTY(EditDefaultsOnly) TArray<FPJStatModifier> Modifiers; // Duration/Infinite용

    UPROPERTY(EditDefaultsOnly) EPJEffectStacking Stacking = EPJEffectStacking::Refresh;
    UPROPERTY(EditDefaultsOnly) int32 MaxStacks = 1;

    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer GrantedTags; // State.*
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer CueTags;     // 표현 트리거
};
