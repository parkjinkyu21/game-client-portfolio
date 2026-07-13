#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "MailContentEvents.generated.h"

// 메일 도메인 이벤트 — 채널 태그와 페이로드 struct는 각각의 도메인에서 정의한다.
// PAContentEvents 네임스페이스는 모든 컨텐츠에서 같이 사용(정의)한다.
namespace PAContentEvents
{
    PACONTENTSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mail_Event_UnreadCount);
}

// 안읽은 메일 수 변경 (채널: Mail.Event.UnreadCount)
// 컨텐츠 및 다른 시스템에서 Mail.Event.UnreadCount 채널을 구독하여 이벤트를 받는다.
USTRUCT()
struct FMailUnreadCountChanged
{
    GENERATED_BODY()

    UPROPERTY()
    int32 UnreadCount = 0;

    FMailUnreadCountChanged() = default;
    explicit FMailUnreadCountChanged(int32 InUnreadCount) : UnreadCount(InUnreadCount) {}
};
