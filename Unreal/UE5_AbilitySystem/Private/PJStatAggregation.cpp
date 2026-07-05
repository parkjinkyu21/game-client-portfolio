#include "PJStatAggregation.h"

float PJStatAggregation::Aggregate(
    float Base, const TArray<FPJStatModifier>& Mods, float MinV, float MaxV)
{
    // 계산 순서를 고정해, 모디파이어가 어떤 순서로 들어왔든 늘 같은 결과가 나오게 한다.
    // (float 합/곱은 계산 순서에 따라 결과가 미세하게 달라지므로)
    // 그래서 (Op, Priority, Source.Id) 순으로 정렬한 사본으로 계산한다.
    TArray<FPJStatModifier> Sorted = Mods;
    Sorted.Sort([](const FPJStatModifier& A, const FPJStatModifier& B)
    {
        if (A.Op != B.Op) return (uint8)A.Op < (uint8)B.Op;
        if (A.Priority != B.Priority) return A.Priority < B.Priority;
        return A.Source.Id < B.Source.Id;
    });

    float AddSum = 0.f;
    float MulProduct = 1.f;
    bool bHasOverride = false;
    float OverrideValue = 0.f;
    int32 OverridePrio = TNumericLimits<int32>::Lowest();
    int32 OverrideSourceId = TNumericLimits<int32>::Lowest();

    for (const FPJStatModifier& M : Sorted)
    {
        switch (M.Op)
        {
        case EPJModifierOp::Additive:       AddSum += M.Magnitude; break;
        case EPJModifierOp::Multiplicative: MulProduct *= (1.f + M.Magnitude); break;
        case EPJModifierOp::Override:
            if (!bHasOverride || M.Priority > OverridePrio ||
                (M.Priority == OverridePrio && M.Source.Id > OverrideSourceId))
            {
                bHasOverride = true;
                OverrideValue = M.Magnitude;
                OverridePrio = M.Priority;
                OverrideSourceId = M.Source.Id;
            }
            break;
        }
    }

    float Result = (Base + AddSum) * MulProduct;
    if (bHasOverride) { Result = OverrideValue; }
    return FMath::Clamp(Result, MinV, MaxV);
}
