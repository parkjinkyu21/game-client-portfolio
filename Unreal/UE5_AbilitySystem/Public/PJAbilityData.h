#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PJAbilityTypes.h"
#include "PJAbilityData.generated.h"

class UPJStatusEffectData;

/**
 * 어빌리티 정의 데이터에셋.
 *
 * [활성화 정책] None / OnGranted(부여 즉시) / OnInputTriggered(입력 1회) / WhileInputActive(누르는 동안)
 * [태그 게이팅] RequiredTags(전부 보유해야 발동) / BlockedTags(하나라도 있으면 차단, 예: State.Stun)
 * [코스트·쿨다운은 효과로 통일]
 *   - 코스트: CostStatTag에서 CostAmount만큼 Instant 차감
 *   - 쿨다운: CooldownTag를 부여하는 Duration 효과(만료 시 자동 해제)
 * [발동 효과] GrantEffectOnActivate: 발동 시 대상에 적용할 버프/디버프
 */
UCLASS(BlueprintType)
class PJABILITYSYSTEM_API UPJAbilityData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) FGameplayTag AbilityTag;
    UPROPERTY(EditDefaultsOnly) EPJAbilityActivation Activation = EPJAbilityActivation::OnInputTriggered;
    UPROPERTY(EditDefaultsOnly) bool bWantsTick = false;

    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer RequiredTags;
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer BlockedTags;

    UPROPERTY(EditDefaultsOnly) FGameplayTag CooldownTag;
    UPROPERTY(EditDefaultsOnly) float CooldownDuration = 0.f;

    UPROPERTY(EditDefaultsOnly) FGameplayTag CostStatTag;
    UPROPERTY(EditDefaultsOnly) float CostAmount = 0.f;

    UPROPERTY(EditDefaultsOnly) TObjectPtr<UPJStatusEffectData> GrantEffectOnActivate = nullptr;
};
