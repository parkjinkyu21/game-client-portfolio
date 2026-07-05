#include "PJStateHash.h"
#include "PJStatusEffect.h"
#include "PJStatusEffectData.h"

uint32 PJStateHash::HashEntry(const FPJEffectHashEntry& E)
{
    uint32 H = GetTypeHash(E.EffectTag);
    H = HashCombine(H, ::GetTypeHash(E.SourceId));
    H = HashCombine(H, ::GetTypeHash(E.StackCount));
    H = HashCombine(H, ::GetTypeHash(E.StartTimeMs));
    return H;
}

uint32 PJStateHash::HashSet(const TArray<FPJEffectHashEntry>& Entries)
{
    // 각 엔트리 해시를 전부 더한다. 더하기는 순서를 타지 않으므로 순서가 달라도 같은 세트면 같은 해시.
    uint32 Sum = 0;
    for (const FPJEffectHashEntry& E : Entries) { Sum += HashEntry(E); }
    return Sum;
}

uint32 PJStateHash::HashActiveEffects(const TArray<TObjectPtr<UPJStatusEffect>>& Effects)
{
    TArray<FPJEffectHashEntry> Entries;
    Entries.Reserve(Effects.Num());
    for (const TObjectPtr<UPJStatusEffect>& E : Effects)
    {
        if (!E || !E->GetData()) continue;
        FPJEffectHashEntry Entry;
        Entry.EffectTag = E->GetData()->EffectTag;
        Entry.SourceId = E->GetSource().Id;
        Entry.StackCount = E->GetStackCount();
        Entry.StartTimeMs = FMath::RoundToInt(E->GetStartTime() * 1000.0);
        Entries.Add(Entry);
    }
    return HashSet(Entries);
}
