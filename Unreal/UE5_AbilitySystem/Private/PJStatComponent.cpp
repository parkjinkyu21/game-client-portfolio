#include "PJStatComponent.h"
#include "PJStatAggregation.h"

void UPJStatComponent::AddStat(const FPJStatDefinition& Def)
{
    FPJStatRuntime S;
    S.Def = Def;
    S.CurrentValue = (Def.Kind == EPJStatKind::Resource) ? Def.Base : 0.f;
    S.bDirty = true;
    Stats.Add(Def.StatTag, S);
}

bool UPJStatComponent::HasStat(FGameplayTag StatTag) const
{
    return Stats.Contains(StatTag);
}

FPJStatRuntime* UPJStatComponent::Find(FGameplayTag Tag) { return Stats.Find(Tag); }

float UPJStatComponent::ComputeMax(const FPJStatRuntime& S)
{
    return PJStatAggregation::Aggregate(S.Def.Base, S.Modifiers, S.Def.MinValue, S.Def.MaxValue);
}

void UPJStatComponent::MarkDirty(FPJStatRuntime& S) { S.bDirty = true; }

float UPJStatComponent::GetValue(FGameplayTag StatTag)
{
    FPJStatRuntime* S = Find(StatTag);
    if (!S) return 0.f;

    if (S->Def.Kind == EPJStatKind::Resource)
    {
        return S->CurrentValue;
    }
    if (S->bDirty)
    {
        S->CachedValue = PJStatAggregation::Aggregate(
            S->Def.Base, S->Modifiers, S->Def.MinValue, S->Def.MaxValue);
        S->bDirty = false;
    }
    return S->CachedValue;
}

float UPJStatComponent::GetMax(FGameplayTag StatTag)
{
    FPJStatRuntime* S = Find(StatTag);
    if (!S) return 0.f;
    return ComputeMax(*S);
}

FPJStatModifierHandle UPJStatComponent::AddModifier(const FPJStatModifier& Mod)
{
    FPJStatModifier Copy = Mod;
    if (!Copy.Source.IsValid()) { Copy.Source = FPJStatModifierHandle::Generate(); }

    FPJStatRuntime* S = Find(Copy.StatTag);
    if (!S) { return FPJStatModifierHandle(); }

    const float Old = GetValue(Copy.StatTag);
    S->Modifiers.Add(Copy);
    MarkDirty(*S);

    // Resource: Max 변동 시 Current 클램프
    if (S->Def.Kind == EPJStatKind::Resource)
    {
        const float NewMax = ComputeMax(*S);
        S->CurrentValue = FMath::Clamp(S->CurrentValue, S->Def.MinValue, NewMax);
    }
    BroadcastIfChanged(Copy.StatTag, Old, GetValue(Copy.StatTag));
    return Copy.Source;
}

void UPJStatComponent::RemoveModifiersBySource(FPJStatModifierHandle Source)
{
    for (auto& Pair : Stats)
    {
        FPJStatRuntime& S = Pair.Value;
        const int32 Removed = S.Modifiers.RemoveAll(
            [&](const FPJStatModifier& M){ return M.Source == Source; });
        if (Removed > 0)
        {
            const float Old = GetValue(Pair.Key);
            MarkDirty(S);
            if (S.Def.Kind == EPJStatKind::Resource)
            {
                const float NewMax = ComputeMax(S);
                S.CurrentValue = FMath::Clamp(S.CurrentValue, S.Def.MinValue, NewMax);
            }
            BroadcastIfChanged(Pair.Key, Old, GetValue(Pair.Key));
        }
    }
}

void UPJStatComponent::ApplyInstant(FGameplayTag StatTag, float Delta)
{
    FPJStatRuntime* S = Find(StatTag);
    if (!S) return;

    const float Old = GetValue(StatTag);
    if (S->Def.Kind == EPJStatKind::Resource)
    {
        const float NewMax = ComputeMax(*S);
        S->CurrentValue = FMath::Clamp(S->CurrentValue + Delta, S->Def.MinValue, NewMax);
    }
    else
    {
        S->Def.Base += Delta;
        MarkDirty(*S);
    }
    BroadcastIfChanged(StatTag, Old, GetValue(StatTag));
}

void UPJStatComponent::BroadcastIfChanged(FGameplayTag Tag, float Old, float New)
{
    if (!FMath::IsNearlyEqual(Old, New))
    {
        OnStatChanged.Broadcast(Tag, Old, New);
    }
}
