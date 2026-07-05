#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PJStatData.h"
#include "PJAbilityTypes.h"
#include "PJStatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPJOnStatChanged, FGameplayTag, StatTag, float, OldValue, float, NewValue);

/**
 * 스탯 컴포넌트 - 캐릭터의 수치(공격력/HP/마나 등)를 GameplayTag로 관리한다.
 *
 * [스탯 종류]
 * - Attribute형(공격력/방어력/이동속도): 값 = 집계(Base, 모디파이어)
 * - Resource형(HP/마나/스태미나): Current(0..Max) + Max = 집계(BaseMax, 모디파이어)
 *
 * [핵심 설계]
 * - 모디파이어 스택: 버프/디버프를 FPJStatModifier로 쌓고 소스 핸들로 일괄 회수(원복 가능)
 * - 페이즈 집계: (Base + ΣAdd) × Π(1+Mul) → Override → Clamp  (PJStatAggregation)
 * - dirty-flag 지연 재계산: 모디파이어 변경 시 표시만, GetValue 시 1회 재계산(매프레임 재계산 제거)
 * - Max 감소(버프 만료) 시 Current를 새 Max로 클램프
 *
 * 네트워크 비의존(순수 로컬 시뮬). 복제/RPC 없음 - 동기화는 PJAbilitySyncComponent가 담당.
 */
UCLASS(ClassGroup=(PJ), meta=(BlueprintSpawnableComponent))
class PJABILITYSYSTEM_API UPJStatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ─── 스탯 정의/조회 ─────────────────────────────────────────────
    void AddStat(const FPJStatDefinition& Def);
    bool HasStat(FGameplayTag StatTag) const;

    UFUNCTION(BlueprintPure, Category="PJ|Stat")
    float GetValue(FGameplayTag StatTag);

    UFUNCTION(BlueprintPure, Category="PJ|Stat")
    float GetMax(FGameplayTag StatTag);

    // ─── 모디파이어 / 즉시 적용 ─────────────────────────────────────
    // 모디파이어 추가(Source 미지정 시 자동 발급). 반환 핸들로 나중에 일괄 제거.
    FPJStatModifierHandle AddModifier(const FPJStatModifier& Mod);
    // 특정 소스(효과)가 건 모든 모디파이어를 스탯 전체에서 회수하고 해당 스탯을 다시 계산
    void RemoveModifiersBySource(FPJStatModifierHandle Source);
    // 즉시 1회 변경: Resource는 Current 이동(클램프), Attribute는 Base 변경(데미지/힐/코스트)
    void ApplyInstant(FGameplayTag StatTag, float Delta);

    UPROPERTY(BlueprintAssignable, Category="PJ|Stat")
    FPJOnStatChanged OnStatChanged;

private:
    UPROPERTY() TMap<FGameplayTag, FPJStatRuntime> Stats;

    FPJStatRuntime* Find(FGameplayTag Tag);
    float ComputeMax(const FPJStatRuntime& S);
    void MarkDirty(FPJStatRuntime& S);
    void BroadcastIfChanged(FGameplayTag Tag, float Old, float New);
};
