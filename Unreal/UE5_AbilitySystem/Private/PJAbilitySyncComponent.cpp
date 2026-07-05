#include "PJAbilitySyncComponent.h"
#include "PJStatusEffectComponent.h"
#include "PJStatusEffect.h"
#include "PJStatusEffectData.h"
#include "PJAbilitySubsystem.h"
#include "PJStateHash.h"
#include "PJAbilityTypes.h"

void UPJAbilitySyncComponent::BuildSnapshot(TArray<FPJEffectSnapshotEntry>& Out) const
{
    Out.Reset();
    if (!EffectComponent) return;
    for (const TObjectPtr<UPJStatusEffect>& E : EffectComponent->GetActiveEffects())
    {
        if (!E || !E->GetData()) continue;
        FPJEffectSnapshotEntry Entry;
        Entry.EffectTag = E->GetData()->EffectTag;
        Entry.SourceId = E->GetSource().Id;
        Entry.StackCount = E->GetStackCount();
        Entry.StartTime = E->GetStartTime();
        Out.Add(Entry);
    }
}

void UPJAbilitySyncComponent::ApplySnapshot(const TArray<FPJEffectSnapshotEntry>& Snapshot)
{
    if (!EffectComponent || !Subsystem) return;

    // 스냅샷에 맞춰 다시 맞춘다(여러 번 적용해도 결과 동일): 기존 효과를 전부 지우고 스냅샷대로 다시 적용
    TArray<TObjectPtr<UPJStatusEffect>> Current = EffectComponent->GetActiveEffects();
    for (const TObjectPtr<UPJStatusEffect>& E : Current)
    {
        if (E) { EffectComponent->RemoveEffect(E->GetSource()); }
    }

    for (const FPJEffectSnapshotEntry& Entry : Snapshot)
    {
        UPJStatusEffectData* Data = Subsystem->FindEffectByTag(Entry.EffectTag);
        if (!Data) continue;
        FPJStatModifierHandle Src; Src.Id = Entry.SourceId;
        EffectComponent->ApplyEffect(Data, Src, Entry.StartTime);
    }
}

uint32 UPJAbilitySyncComponent::ComputeLocalHash() const
{
    if (!EffectComponent) return 0;
    return PJStateHash::HashActiveEffects(EffectComponent->GetActiveEffects());
}

bool UPJAbilitySyncComponent::NeedsRepair(uint32 AuthorityHash) const
{
    return ComputeLocalHash() != AuthorityHash;
}
