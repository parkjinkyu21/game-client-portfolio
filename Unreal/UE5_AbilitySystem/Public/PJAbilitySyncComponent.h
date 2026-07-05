#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PJAbilitySyncComponent.generated.h"

class UPJStatusEffectComponent;
class UPJAbilitySubsystem;

USTRUCT()
struct FPJEffectSnapshotEntry
{
    GENERATED_BODY()
    UPROPERTY() FGameplayTag EffectTag;
    UPROPERTY() int32 SourceId = 0;
    UPROPERTY() int32 StackCount = 1;
    UPROPERTY() double StartTime = 0.0;
};

/**
 * 동기화 컴포넌트 - 코어(효과/스탯) 위에 얹히는 얇은 네트워크 계층.
 * 코어는 이 컴포넌트를 전혀 모른다(코어의 네트워크 비의존 유지).
 *
 * [모델] 이벤트 재생 + relevancy(관련성) 스냅샷 + 상태 해시로 desync 감지 (경쟁전이 아닌 협동/PvE 환경 기준)
 *  - BuildSnapshot/ApplySnapshot: 접속이나 관련성 확보 시 현재 활성 세트를 통째로 다시 맞춘다.
 *    같은 스냅샷을 여러 번 적용해도 결과가 같아서 중복이나 유실에 안전하다.
 *  - ComputeLocalHash: 활성 효과 세트를 정규화해 해시로 만든다(PJStateHash).
 *  - NeedsRepair(서버 해시): 로컬 해시가 서버 해시와 다르면 desync로 보고 재스냅샷으로 교정한다.
 *    전체 상태를 상시 복제하지 않고, 해시가 어긋날 때만 다시 맞춘다.
 */
UCLASS(ClassGroup=(PJ), meta=(BlueprintSpawnableComponent))
class PJABILITYSYSTEM_API UPJAbilitySyncComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void SetStatusEffectComponent(UPJStatusEffectComponent* In) { EffectComponent = In; }
    void SetSubsystem(UPJAbilitySubsystem* In) { Subsystem = In; }

    void BuildSnapshot(TArray<FPJEffectSnapshotEntry>& Out) const;
    void ApplySnapshot(const TArray<FPJEffectSnapshotEntry>& Snapshot);
    uint32 ComputeLocalHash() const;
    bool NeedsRepair(uint32 AuthorityHash) const;

private:
    UPROPERTY() TObjectPtr<UPJStatusEffectComponent> EffectComponent = nullptr;
    UPROPERTY() TObjectPtr<UPJAbilitySubsystem> Subsystem = nullptr;
};
