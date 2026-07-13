#pragma once

#include "CoreMinimal.h"

// 컨텐츠 네트워크 메시지(순수 C++).
// 파생 메시지의 의무:
//   GetMessageId()                  — 메시지 id "{도메인}.{동작}".
//                                     요청은 서버 핸들러를, 노티는 수신 리스너를 이 id로 찾는다. 응답에선 디버그용
//   Serialize(FArchive&)            — 모든 필드를 Ar << 로 (저장/로드 겸용)
//   필드 추가 시 Serialize 갱신 필수 — 빼먹으면 데이터 누락됨.
// 요청은 FPAContentMessage 대신 TPAContentRequest<응답타입>을 사용한다.
struct PACONTENTSYSTEM_API FPAContentMessage
{
    virtual ~FPAContentMessage() = default;

    virtual FName GetMessageId() const = 0;
    virtual void Serialize(FArchive& Ar) = 0;
};

// 요청 메시지 베이스 — Response 타입은 요청 메시지를 정의할 때 템플릿 인자로 정의된다.
// Bind/Send 함수들이 FResponse 타입으로 Response 메시지를 만들어 콜백에 전달한다. (UPAContentNetworkBase::DispatchResponse)
template <typename TResponse>
struct TPAContentRequest : public FPAContentMessage
{
    using FResponse = TResponse;
};
