#pragma once

#include "CoreMinimal.h"
#include "PAContentTypes.generated.h"

// 캐시 재취득 정책: 언제 "다시" 받는가
UENUM()
enum class EPAContentCachePolicy : uint8
{
    CacheUntilInvalidated, // 세션 내 1회 취득. InvalidateCache() 호출 시에만 재취득
    CacheWithLifetime,     // CacheLifetimeSeconds 경과 시 재취득
    NoCache,               // 매번 재취득
};

// 요청 결과. 실패 사유 세분화(Timeout/서버 에러코드 등)는 실서버 네트워크 구현 도입 시
// enum 확장으로 — 값 추가는 비파괴 변경이라 미리 만들지 않는다.
UENUM()
enum class EPAContentRequestResult : uint8
{
    Success,
    Failed,
};
