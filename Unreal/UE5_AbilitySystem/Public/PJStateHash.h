#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UPJStatusEffect;

// 활성 효과 하나를 해시하기 위한 정규화 엔트리.
// 목적은 고유성이 아니라 "완전성" - 바뀔 수 있는 상태를 모두 담아야 desync를 놓치지 않는다.
//  - EffectTag + SourceId : 인스턴스 식별(멀티소스/ms충돌 구분). 식별자는 Source이지 StartTime이 아님.
//  - StackCount           : 스택 desync 감지(tag+start만으론 못 잡는 대표 누락)
//  - StartTimeMs          : refresh/재적용 감지(ms 반올림으로 float 노이즈 제거)
// 매 순간 계산되는 값(남은시간·dirty 플래그·캐시된 결과)은 넣지 않는다.
// 서버·클라가 각자 계산하는 시점이 달라 값이 갈리므로, 실제로는 동기화됐는데도
// 해시가 어긋나 '가짜 desync'로 잘못 판정하게 된다.
struct PJABILITYSYSTEM_API FPJEffectHashEntry
{
    FGameplayTag EffectTag;
    int32 SourceId = 0;
    int32 StackCount = 1;
    int32 StartTimeMs = 0;
};

// 상태 해시 순수 함수.
// 세트 해시 = 각 엔트리 해시를 전부 더한 값. 더하기는 순서를 타지 않으므로,
// 효과가 들어온 순서가 달라도 같은 세트면 같은 해시가 나온다.
namespace PJStateHash
{
    PJABILITYSYSTEM_API uint32 HashEntry(const FPJEffectHashEntry& E);
    PJABILITYSYSTEM_API uint32 HashSet(const TArray<FPJEffectHashEntry>& Entries);
    PJABILITYSYSTEM_API uint32 HashActiveEffects(const TArray<TObjectPtr<UPJStatusEffect>>& Effects);
}
