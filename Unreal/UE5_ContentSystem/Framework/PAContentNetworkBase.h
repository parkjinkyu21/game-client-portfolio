#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "PAContentMessage.h"
#include "PAContentTypes.h"
#include "PAContentNetworkBase.generated.h"

// 메시지 바이트 델리게이트 (내부 계약 — 등록은 typed Register* 사용)
DECLARE_DELEGATE_TwoParams(FPAContentMessageDelegate, EPAContentRequestResult /*Result*/, const TArray<uint8>& /*MessageBytes — Success일 때만 유효*/);

// 컨텐츠관련 서버 통신을 위한 클래스. 요청/응답/노티.
// 수신 메시지는 메시지 id로 등록된 핸들러(메시지당 1개)를 찾아 전달한다.
// 타임아웃/재시도는 고려되지 않았음.
// 수신 규칙: 응답/노티는 수신 버퍼에 쌓였다가 게임 스레드에서 다음 월드 틱 시작 직전에 도착 순서대로 일괄 처리된다.

UCLASS(Abstract)
class PACONTENTSYSTEM_API UPAContentNetworkBase : public UObject
{
    GENERATED_BODY()

public:
    // 응답 핸들러 등록 — 메시지당 1개, 중복 등록은 Error 로그 후 무시. Owner가 파괴되면 콜백은 오지 않는다.
    // 응답은 TRequest::FResponse로 복원해 전달한다 (짝은 컴파일 타임 강제). 실패 시 기본 생성 FResponse가 온다.
    template <typename TRequest>
    void RegisterResponseHandler(UObject* Owner,
        TDelegate<void(EPAContentRequestResult, const typename TRequest::FResponse&)> Handler)
    {
        RegisterMessageHandlerBytes(TRequest().GetMessageId(),
            FPAContentMessageDelegate::CreateStatic(&UPAContentNetworkBase::DispatchResponse<TRequest>,
                TWeakObjectPtr<UObject>(Owner), MoveTemp(Handler)));
    }

    // 노티 핸들러 등록 — 메시지당 1개, 중복 등록은 Error 로그 후 무시. Owner가 파괴되면 콜백은 오지 않는다.
    template <typename TNotify>
    void RegisterNotifyHandler(UObject* Owner, TDelegate<void(const TNotify&)> Handler)
    {
        RegisterMessageHandlerBytes(TNotify().GetMessageId(),
            FPAContentMessageDelegate::CreateStatic(&UPAContentNetworkBase::DispatchNotify<TNotify>,
                TWeakObjectPtr<UObject>(Owner), MoveTemp(Handler)));
    }

    // 요청을 직렬화해 전송한다. 응답은 등록된 응답 핸들러로 온다.
    template <typename TRequest>
    void SendRequest(TRequest Request)
    {
        TArray<uint8> RequestBytes;
        FMemoryWriter Writer(RequestBytes);
        Request.Serialize(Writer);

        SendRequestBytes(Request.GetMessageId(), MoveTemp(RequestBytes));
    }

    // 수신 버퍼 일괄 처리 — 월드 틱 시작 시점에 매 프레임 호출된다. 테스트는 직접 호출해 월드 틱을 시뮬레이션.
    // 처리 중 새로 도착한 메시지(콜백發 재요청 등)는 다음 처리로 넘긴다 — 동기 응답 방지.
    void ProcessReceiveBuffer();

    // 수신 버퍼 제거 (콜백 없음). 로그아웃 시 이전 세션 메시지 차단용.
    virtual void CancelAllRequests();
    // 생성 직후 1회: OnWorldTickStart 델리게이트에 ProcessReceiveBuffer를 연결한다 (해제는 BeginDestroy)
    virtual void PostInitProperties() override;
    virtual void BeginDestroy() override;

protected:
    // 파생 구현 지점 — 요청 바이트를 서버로 전송한다.
    virtual void SendRequestBytes(FName MessageId, TArray<uint8>&& RequestBytes)
        PURE_VIRTUAL(UPAContentNetworkBase::SendRequestBytes, );

    // 파생 클래스의 네트워크 수신부가 받은 메시지를 버퍼에 담는다. 노티 메시지의 경우에는 Result = Success으로 한다.
    void ReceiveMessage(EPAContentRequestResult Result, FName MessageId, TArray<uint8>&& MessageBytes);

private:
    // 수신 버퍼 항목 — 도착 순서대로 보관
    struct FReceivedPacket
    {
        EPAContentRequestResult Result = EPAContentRequestResult::Failed;
        FName MessageId;
        TArray<uint8> Bytes;
    };

    // 수신 바이트를 응답 타입으로 복원해 핸들러에 전달한다. Owner 파괴 시 호출하지 않는다.
    template <typename TRequest>
    static void DispatchResponse(EPAContentRequestResult Result, const TArray<uint8>& MessageBytes,
        TWeakObjectPtr<UObject> WeakOwner,
        TDelegate<void(EPAContentRequestResult, const typename TRequest::FResponse&)> Handler)
    {
        if (!WeakOwner.IsValid())
        {
            return;
        }

        typename TRequest::FResponse Response;
        if (Result == EPAContentRequestResult::Success)
        {
            FMemoryReader Reader(MessageBytes);
            Response.Serialize(Reader);
        }
        Handler.ExecuteIfBound(Result, Response);
    }

    // 수신 바이트를 노티 타입으로 복원해 핸들러에 전달한다. Owner 파괴 시 호출하지 않는다.
    template <typename TNotify>
    static void DispatchNotify(EPAContentRequestResult /*노티는 결과 없음*/, const TArray<uint8>& MessageBytes,
        TWeakObjectPtr<UObject> WeakOwner, TDelegate<void(const TNotify&)> Handler)
    {
        if (!WeakOwner.IsValid())
        {
            return;
        }

        TNotify Notify;
        FMemoryReader Reader(MessageBytes);
        Notify.Serialize(Reader);
        Handler.ExecuteIfBound(Notify);
    }

    void RegisterMessageHandlerBytes(FName MessageId, FPAContentMessageDelegate Handler);

    // 메시지 핸들러 — 메시지 id당 1개
    TMap<FName, FPAContentMessageDelegate> MessageHandlers;

    // 수신 스레드가 게임 스레드와 다르면 이 버퍼에 접근하는 모든 곳
    // (ReceiveMessage의 추가, ProcessReceiveBuffer의 스왑)을 같은 CriticalSection으로 가드해야 한다.
    TArray<FReceivedPacket> ReceiveBuffer;
    FDelegateHandle WorldTickStartHandle;
};
