#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PJAbilityTypes.h"
#include "IPJTimeSource.h"
#include "PJStatusEffectComponent.generated.h"

class UPJStatComponent;
class UPJStatusEffect;
class UPJStatusEffectData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJOnStatusEffectChanged, FGameplayTag, EffectTag);

/**
 * 상태효과 컴포넌트 - 활성 효과 목록을 관리하고 스탯에 반영한다.
 *
 * [주요 동작]
 * - ApplyEffect: 스택/리프레시 정책을 처리한 뒤, Instant면 스탯에 즉시 적용하고
 *   Duration·Infinite면 런타임 인스턴스를 만들어 모디파이어를 주입한다.
 * - TickEffects: 주기 효과를 적용(서버 시간 경계)하고 만료된 효과를 회수한다.
 * - RemoveEffect: 소스 핸들로 스탯 모디파이어까지 함께 정리한다.
 *
 * PJStatComponent를 한 방향으로만 참조한다(효과가 스탯을 바꾼다). 네트워크 비의존.
 * OnEffectAdded/Removed는 연출(VFX/SFX)을 붙일 수 있는 델리게이트다.
 */
UCLASS(ClassGroup=(PJ), meta=(BlueprintSpawnableComponent))
class PJABILITYSYSTEM_API UPJStatusEffectComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void SetStatComponent(UPJStatComponent* InStat) { StatComponent = InStat; }
    void SetTimeSource(TSharedPtr<IPJTimeSource> InTime) { TimeSource = InTime; }

    FPJStatModifierHandle ApplyEffect(UPJStatusEffectData* Data,
        FPJStatModifierHandle Source, double StartServerTime);
    void RemoveEffect(FPJStatModifierHandle Source);
    void TickEffects();

    bool HasEffectTag(FGameplayTag EffectTag) const;
    const TArray<TObjectPtr<UPJStatusEffect>>& GetActiveEffects() const { return ActiveEffects; }
    FGameplayTagContainer GetGrantedTags() const;

    UPROPERTY(BlueprintAssignable) FPJOnStatusEffectChanged OnEffectAdded;
    UPROPERTY(BlueprintAssignable) FPJOnStatusEffectChanged OnEffectRemoved;

private:
    UPROPERTY() TObjectPtr<UPJStatComponent> StatComponent = nullptr;
    UPROPERTY() TArray<TObjectPtr<UPJStatusEffect>> ActiveEffects;
    TSharedPtr<IPJTimeSource> TimeSource;

    UPJStatusEffect* FindByEffectTag(FGameplayTag EffectTag) const;
    void InjectModifiers(UPJStatusEffect* Effect);
};
