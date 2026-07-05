#pragma once
#include "CoreMinimal.h"
#include "PJAbilityTypes.h"

// 스탯 최종값 집계 함수
// 컴포넌트에서 분리해 로직만 독립 검증한다.
namespace PJStatAggregation
{
    // 항상 같은 결과가 나오도록 아래 순서로 계산한다.
    //  1) 모디파이어를 (Op, Priority, Source.Id) 순으로 정렬해 연산 순서를 고정(들어온 순서와 무관하게 같은 결과)
    //  2) result = (Base + Σ Additive) × Π(1 + Multiplicative)
    //  3) Override 존재 시 최우선(Priority 최상위, 동률이면 Source.Id 큰 쪽)값으로 대체
    //  4) [MinV, MaxV]로 Clamp
    PJABILITYSYSTEM_API float Aggregate(
        float Base, const TArray<FPJStatModifier>& Mods, float MinV, float MaxV);
}
