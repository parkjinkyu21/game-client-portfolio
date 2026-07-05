#include "PJAbilityComponent.h"
#include "PJAbility.h"
#include "PJAbilityData.h"
#include "PJStatComponent.h"
#include "PJStatusEffectComponent.h"
#include "PJStatusEffectData.h"
#include "PJAbilityTypes.h"
#include "IPJTimeSource.h"

double UPJAbilityComponent::Now() const
{
    return TimeSource.IsValid() ? TimeSource->GetTimeSeconds() : 0.0;
}

FGameplayTagContainer UPJAbilityComponent::GatherOwnerTags() const
{
    FGameplayTagContainer Tags;
    if (StatusEffectComponent) { Tags.AppendTags(StatusEffectComponent->GetGrantedTags()); }
    return Tags;
}

void UPJAbilityComponent::GrantAbility(UPJAbilityData* Data)
{
    if (!Data || !Data->AbilityTag.IsValid()) return;
    UPJAbility* Ability = NewObject<UPJAbility>(this);
    Ability->Init(Data);
    Abilities.Add(Data->AbilityTag, Ability);

    if (Data->Activation == EPJAbilityActivation::OnGranted)
    {
        ActivateAbility(Data->AbilityTag);
    }
}

bool UPJAbilityComponent::HasAbility(FGameplayTag AbilityTag) const
{
    return Abilities.Contains(AbilityTag);
}

bool UPJAbilityComponent::ActivateAbility(FGameplayTag AbilityTag)
{
    TObjectPtr<UPJAbility>* Found = Abilities.Find(AbilityTag);
    if (!Found || !*Found) return false;
    UPJAbility* Ability = *Found;
    UPJAbilityData* Data = Ability->GetData();
    if (!Data) return false;

    if (!Ability->CanActivate(GatherOwnerTags(), StatComponent)) return false;

    // 코스트 차감
    if (StatComponent && Data->CostStatTag.IsValid() && Data->CostAmount > 0.f)
    {
        StatComponent->ApplyInstant(Data->CostStatTag, -Data->CostAmount);
    }

    // 쿨다운: CooldownTag를 담은 Duration 효과로 처리
    if (StatusEffectComponent && Data->CooldownTag.IsValid() && Data->CooldownDuration > 0.f)
    {
        UPJStatusEffectData* CD = NewObject<UPJStatusEffectData>(this);
        CD->EffectTag = Data->CooldownTag;
        CD->DurationPolicy = EPJEffectDuration::Duration;
        CD->Duration = Data->CooldownDuration;
        CD->Stacking = EPJEffectStacking::Refresh;
        CD->GrantedTags.AddTag(Data->CooldownTag);
        StatusEffectComponent->ApplyEffect(CD, FPJStatModifierHandle::Generate(), Now());
    }

    // 효과 부여
    if (StatusEffectComponent && Data->GrantEffectOnActivate)
    {
        StatusEffectComponent->ApplyEffect(
            Data->GrantEffectOnActivate, FPJStatModifierHandle::Generate(), Now());
    }

    Ability->Activate();
    return true;
}

void UPJAbilityComponent::DeactivateAbility(FGameplayTag AbilityTag)
{
    TObjectPtr<UPJAbility>* Found = Abilities.Find(AbilityTag);
    if (Found && *Found) { (*Found)->Deactivate(); }
}

void UPJAbilityComponent::TickAbilities()
{
    for (auto& Pair : Abilities)
    {
        UPJAbility* A = Pair.Value;
        if (A && A->IsActive() && A->WantsTick())
        {
            // 틱 어빌리티의 지속 로직 (파생에서 확장)
        }
    }
}
