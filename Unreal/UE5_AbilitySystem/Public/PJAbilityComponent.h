#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "IPJTimeSource.h"
#include "PJAbilityComponent.generated.h"

class UPJAbilityData;
class UPJAbility;
class UPJStatComponent;
class UPJStatusEffectComponent;

/**
 * 어빌리티 컴포넌트 - 어빌리티 부여/발동/틱을 관리한다.
 *
 * ActivateAbility(Tag):
 *   1) CanActivate 검사(오너 태그 요건/차단, 쿨다운, 코스트 충분 여부)
 *   2) 코스트를 차감하고, 쿨다운 효과를 부여한 뒤, 발동 효과를 적용
 *   3) 어빌리티 활성화
 * 오너 태그는 StatusEffectComponent의 활성 효과 GrantedTags에서 수집(State.Stun 등으로 차단).
 * 네트워크 비의존 - 발동은 동기화 계층이 각 머신에서 동일하게 호출.
 */
UCLASS(ClassGroup=(PJ), meta=(BlueprintSpawnableComponent))
class PJABILITYSYSTEM_API UPJAbilityComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void SetStatComponent(UPJStatComponent* In) { StatComponent = In; }
    void SetStatusEffectComponent(UPJStatusEffectComponent* In) { StatusEffectComponent = In; }
    void SetTimeSource(TSharedPtr<IPJTimeSource> In) { TimeSource = In; }

    void GrantAbility(UPJAbilityData* Data);
    bool HasAbility(FGameplayTag AbilityTag) const;
    bool ActivateAbility(FGameplayTag AbilityTag);
    void DeactivateAbility(FGameplayTag AbilityTag);
    void TickAbilities();

private:
    UPROPERTY() TMap<FGameplayTag, TObjectPtr<UPJAbility>> Abilities;
    UPROPERTY() TObjectPtr<UPJStatComponent> StatComponent = nullptr;
    UPROPERTY() TObjectPtr<UPJStatusEffectComponent> StatusEffectComponent = nullptr;
    TSharedPtr<IPJTimeSource> TimeSource;

    FGameplayTagContainer GatherOwnerTags() const;
    double Now() const;
};
