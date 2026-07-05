#include "PJAbility.h"
#include "PJAbilityData.h"
#include "PJStatComponent.h"

void UPJAbility::Init(UPJAbilityData* InData) { Data = InData; bActive = false; }

bool UPJAbility::WantsTick() const { return Data && Data->bWantsTick; }

bool UPJAbility::CanActivate(const FGameplayTagContainer& OwnerTags, UPJStatComponent* Stat) const
{
    if (!Data) return false;
    if (Data->RequiredTags.Num() > 0 && !OwnerTags.HasAll(Data->RequiredTags)) return false;
    if (Data->BlockedTags.Num() > 0 && OwnerTags.HasAny(Data->BlockedTags)) return false;
    if (Data->CooldownTag.IsValid() && OwnerTags.HasTag(Data->CooldownTag)) return false;
    if (Stat && Data->CostStatTag.IsValid() && Data->CostAmount > 0.f)
    {
        if (Stat->GetValue(Data->CostStatTag) < Data->CostAmount) return false;
    }
    return true;
}

bool UPJAbility::Activate() { bActive = true; return true; }
void UPJAbility::Deactivate() { bActive = false; }
