#include "PJStatusEffectComponent.h"
#include "PJStatusEffect.h"
#include "PJStatusEffectData.h"
#include "PJStatComponent.h"
#include "IPJTimeSource.h"

UPJStatusEffect* UPJStatusEffectComponent::FindByEffectTag(FGameplayTag EffectTag) const
{
    for (UPJStatusEffect* E : ActiveEffects)
    {
        if (E && E->GetData() && E->GetData()->EffectTag == EffectTag) return E;
    }
    return nullptr;
}

bool UPJStatusEffectComponent::HasEffectTag(FGameplayTag EffectTag) const
{
    return FindByEffectTag(EffectTag) != nullptr;
}

void UPJStatusEffectComponent::InjectModifiers(UPJStatusEffect* Effect)
{
    if (!StatComponent || !Effect || !Effect->GetData()) return;
    for (const FPJStatModifier& Template : Effect->GetData()->Modifiers)
    {
        FPJStatModifier Mod = Template;
        Mod.Magnitude *= Effect->GetStackCount();
        Mod.Source = Effect->GetSource();      // 효과의 소스 핸들로 통일해, 효과를 제거하면 이 모디파이어들도 함께 제거되게 함
        StatComponent->AddModifier(Mod);
    }
}

FPJStatModifierHandle UPJStatusEffectComponent::ApplyEffect(
    UPJStatusEffectData* Data, FPJStatModifierHandle Source, double StartServerTime)
{
    if (!Data) return FPJStatModifierHandle();

    // 스택/리프레시 정책
    if (UPJStatusEffect* Existing = FindByEffectTag(Data->EffectTag))
    {
        switch (Data->Stacking)
        {
        case EPJEffectStacking::None:
            return Existing->GetSource();               // 무시
        case EPJEffectStacking::Refresh:
            Existing->Refresh(StartServerTime);
            return Existing->GetSource();
        case EPJEffectStacking::Stack:
            if (StatComponent) { StatComponent->RemoveModifiersBySource(Existing->GetSource()); }
            Existing->AddStack();
            Existing->Refresh(StartServerTime);
            InjectModifiers(Existing);                  // 스택 반영 재주입
            return Existing->GetSource();
        }
    }

    // Instant: 즉시 1회 적용, 활성 목록에 남기지 않음
    if (Data->DurationPolicy == EPJEffectDuration::Instant)
    {
        if (StatComponent)
        {
            for (const FPJStatModifier& Mod : Data->Modifiers)
            {
                StatComponent->ApplyInstant(Mod.StatTag, Mod.Magnitude);
            }
        }
        return Source;
    }

    // Duration/Infinite: 런타임 생성 + 모디파이어 주입
    UPJStatusEffect* Effect = NewObject<UPJStatusEffect>(this);
    Effect->Init(Data, Source, StartServerTime, TimeSource);
    ActiveEffects.Add(Effect);
    InjectModifiers(Effect);
    OnEffectAdded.Broadcast(Data->EffectTag);
    return Source;
}

void UPJStatusEffectComponent::RemoveEffect(FPJStatModifierHandle Source)
{
    for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
    {
        UPJStatusEffect* E = ActiveEffects[i];
        if (E && E->GetSource() == Source)
        {
            if (StatComponent) { StatComponent->RemoveModifiersBySource(Source); }
            const FGameplayTag Tag = E->GetData() ? E->GetData()->EffectTag : FGameplayTag();
            ActiveEffects.RemoveAt(i);
            OnEffectRemoved.Broadcast(Tag);
        }
    }
}

void UPJStatusEffectComponent::TickEffects()
{
    for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
    {
        UPJStatusEffect* E = ActiveEffects[i];
        if (!E) { ActiveEffects.RemoveAt(i); continue; }

        // 주기 적용(서버 시간 경계 기준)
        if (StatComponent && E->GetData() && E->GetData()->Period > 0.f)
        {
            const int32 Due = E->ConsumeDuePeriods();
            for (int32 p = 0; p < Due; ++p)
            {
                StatComponent->ApplyInstant(E->GetData()->PeriodStatTag,
                    E->GetData()->PeriodMagnitude * E->GetStackCount());
            }
        }

        // 만료 회수
        if (E->IsExpired())
        {
            const FPJStatModifierHandle Src = E->GetSource();
            const FGameplayTag Tag = E->GetData() ? E->GetData()->EffectTag : FGameplayTag();
            if (StatComponent) { StatComponent->RemoveModifiersBySource(Src); }
            ActiveEffects.RemoveAt(i);
            OnEffectRemoved.Broadcast(Tag);
        }
    }
}

FGameplayTagContainer UPJStatusEffectComponent::GetGrantedTags() const
{
    FGameplayTagContainer Out;
    for (UPJStatusEffect* E : ActiveEffects)
    {
        if (E && E->GetData()) { Out.AppendTags(E->GetData()->GrantedTags); }
    }
    return Out;
}
