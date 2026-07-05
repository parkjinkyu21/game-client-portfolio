#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PJAbilityTypes.h"
#include "PJStatData.generated.h"

// 스탯 초기 정의(에디터/데이터에서 설정). 태그·종류·기본값·범위.
USTRUCT(BlueprintType)
struct FPJStatDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly) FGameplayTag StatTag;
    UPROPERTY(EditDefaultsOnly) EPJStatKind Kind = EPJStatKind::Attribute;
    UPROPERTY(EditDefaultsOnly) float Base = 0.f;
    UPROPERTY(EditDefaultsOnly) float MinValue = 0.f;
    UPROPERTY(EditDefaultsOnly) float MaxValue = TNumericLimits<float>::Max();
};

// 스탯 런타임 상태. 정의 + Current(Resource용) + 모디파이어 목록 + dirty 캐시.
USTRUCT()
struct FPJStatRuntime
{
    GENERATED_BODY()
    UPROPERTY() FPJStatDefinition Def;
    UPROPERTY() float CurrentValue = 0.f;   // Resource형 전용(0..Max)
    UPROPERTY() TArray<FPJStatModifier> Modifiers;
    bool bDirty = true;
    float CachedValue = 0.f;
};
