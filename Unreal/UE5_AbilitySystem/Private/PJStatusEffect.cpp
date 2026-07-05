#include "PJStatusEffect.h"
#include "PJStatusEffectData.h"
#include "IPJTimeSource.h"

void UPJStatusEffect::Init(UPJStatusEffectData* InData, FPJStatModifierHandle InSource,
                           double InStartTime, TSharedPtr<IPJTimeSource> InTimeSource)
{
    Data = InData;
    Source = InSource;
    StartTime = InStartTime;
    TimeSource = InTimeSource;
    StackCount = 1;
    PeriodsApplied = 0;
}

double UPJStatusEffect::Now() const
{
    return TimeSource.IsValid() ? TimeSource->GetTimeSeconds() : 0.0;
}

double UPJStatusEffect::GetRemaining() const
{
    if (!Data) return 0.0;
    if (Data->DurationPolicy == EPJEffectDuration::Instant) return 0.0;
    if (Data->DurationPolicy == EPJEffectDuration::Infinite) return TNumericLimits<double>::Max();
    return (double)Data->Duration - (Now() - StartTime);
}

bool UPJStatusEffect::IsExpired() const
{
    if (!Data || Data->DurationPolicy != EPJEffectDuration::Duration) return false;
    return GetRemaining() <= 0.0;
}

void UPJStatusEffect::Refresh(double NewStartTime)
{
    StartTime = NewStartTime;
    PeriodsApplied = 0;
}

void UPJStatusEffect::AddStack()
{
    if (Data) { StackCount = FMath::Min(StackCount + 1, FMath::Max(1, Data->MaxStacks)); }
}

int32 UPJStatusEffect::ConsumeDuePeriods()
{
    if (!Data || Data->Period <= 0.f) return 0;
    const double Elapsed = Now() - StartTime;
    if (Elapsed <= 0.0) return 0;
    const int32 TotalDue = FMath::FloorToInt(Elapsed / (double)Data->Period);
    const int32 New = TotalDue - PeriodsApplied;
    if (New > 0) { PeriodsApplied = TotalDue; return New; }
    return 0;
}
