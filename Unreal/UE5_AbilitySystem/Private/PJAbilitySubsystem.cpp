#include "PJAbilitySubsystem.h"
#include "PJAbilityData.h"
#include "PJStatusEffectData.h"

void UPJAbilitySubsystem::RegisterAbility(UPJAbilityData* Data)
{
    if (Data && Data->AbilityTag.IsValid()) { AbilityRegistry.Add(Data->AbilityTag, Data); }
}

void UPJAbilitySubsystem::RegisterEffect(UPJStatusEffectData* Data)
{
    if (Data && Data->EffectTag.IsValid()) { EffectRegistry.Add(Data->EffectTag, Data); }
}

UPJAbilityData* UPJAbilitySubsystem::FindAbilityByTag(FGameplayTag AbilityTag) const
{
    const TObjectPtr<UPJAbilityData>* Found = AbilityRegistry.Find(AbilityTag);
    return Found ? *Found : nullptr;
}

UPJStatusEffectData* UPJAbilitySubsystem::FindEffectByTag(FGameplayTag EffectTag) const
{
    const TObjectPtr<UPJStatusEffectData>* Found = EffectRegistry.Find(EffectTag);
    return Found ? *Found : nullptr;
}
