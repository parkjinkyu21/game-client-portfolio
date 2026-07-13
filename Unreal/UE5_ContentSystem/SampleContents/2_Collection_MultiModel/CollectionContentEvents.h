#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "CollectionContentEvents.generated.h"

// 컬렉션 도메인 이벤트 — 채널 태그와 페이로드 struct는 각각의 도메인에서 정의한다.
// PAContentEvents 네임스페이스는 모든 컨텐츠에서 같이 사용(정의)한다.
namespace PAContentEvents
{
    PACONTENTSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Collection_Event_Progress);
}

// 수집 진행 변경 (채널: Collection.Event.Progress)
// 컨텐츠 및 다른 시스템에서 Collection.Event.Progress 채널을 구독하여 이벤트를 받는다.
USTRUCT()
struct FCollectionProgressChanged
{
    GENERATED_BODY()

    UPROPERTY()
    int32 AppearanceCollected = 0;

    UPROPERTY()
    int32 AppearanceTotal = 0;

    UPROPERTY()
    int32 TitleOwned = 0;

    UPROPERTY()
    int32 TitleTotal = 0;

    FCollectionProgressChanged() = default;
    explicit FCollectionProgressChanged(int32 InAppearanceCollected, int32 InAppearanceTotal,
        int32 InTitleOwned, int32 InTitleTotal)
        : AppearanceCollected(InAppearanceCollected)
        , AppearanceTotal(InAppearanceTotal)
        , TitleOwned(InTitleOwned)
        , TitleTotal(InTitleTotal)
    {
    }
};
