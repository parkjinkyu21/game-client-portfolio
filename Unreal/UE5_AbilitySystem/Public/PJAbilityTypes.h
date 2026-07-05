#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PJAbilityTypes.generated.h"

// PJ 어빌리티 시스템 공용 값 타입(enum + 모디파이어). GameplayTag로 식별하는 데이터 주도 구조.

// 스탯 모디파이어 연산. 집계할 때 Additive, Multiplicative, Override 순으로 적용한다.
UENUM(BlueprintType)
enum class EPJModifierOp : uint8 { Additive, Multiplicative, Override };

// 스탯 종류. Attribute=집계값이 곧 값 / Resource=Current(0..Max) 보유(HP/마나).
UENUM(BlueprintType)
enum class EPJStatKind : uint8 { Attribute, Resource };

// 어빌리티 활성화 정책. 부여 즉시/입력 1회/입력 유지 동안 등 발동 방식을 지정.
UENUM(BlueprintType)
enum class EPJAbilityActivation : uint8 { None, OnGranted, OnInputTriggered, WhileInputActive };

// 효과 지속 정책. Instant=즉시1회 / Duration=시한 / Infinite=수동제거.
UENUM(BlueprintType)
enum class EPJEffectDuration : uint8 { Instant, Duration, Infinite };

// 재적용 정책. None=무시 / Refresh=StartTime 갱신 / Stack=중첩(MaxStacks).
UENUM(BlueprintType)
enum class EPJEffectStacking : uint8 { None, Refresh, Stack };

// 모디파이어 회수 키. 하나의 소스(효과)가 건 모든 모디파이어를 이 핸들로 일괄 제거한다.
USTRUCT(BlueprintType)
struct FPJStatModifierHandle
{
    GENERATED_BODY()

    UPROPERTY() int32 Id = INDEX_NONE;

    bool IsValid() const { return Id != INDEX_NONE; }
    bool operator==(const FPJStatModifierHandle& O) const { return Id == O.Id; }

    static FPJStatModifierHandle Generate()
    {
        static int32 Counter = 0;
        FPJStatModifierHandle H; H.Id = ++Counter; return H;
    }
};

FORCEINLINE uint32 GetTypeHash(const FPJStatModifierHandle& H) { return ::GetTypeHash(H.Id); }

USTRUCT(BlueprintType)
struct FPJStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly) FGameplayTag StatTag;
    UPROPERTY(EditDefaultsOnly) EPJModifierOp Op = EPJModifierOp::Additive;
    UPROPERTY(EditDefaultsOnly) float Magnitude = 0.f;
    UPROPERTY(EditDefaultsOnly) int32 Priority = 0;
    UPROPERTY() FPJStatModifierHandle Source;
};
