#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PAContentNetworkBase.h"
#include "PAContentTypes.h"
#include "PAContentModelBase.generated.h"

class UPAContentModelManagerBase;
class UPAContentEventDispatcher;

// 컨텐츠 도메인 모델 베이스: 도메인 데이터를 단독 소유하고 변경(OnDataChanged)을 통지한다.
// 모델은 UI만의 데이터가 아니며 게임플레이 필요한 데이터도 포함된다.
//
// 파생 모델 작성:
//   생성자             — CachePolicy 지정, 자식 모델이면 ParentModelClass 지정 (등록은 매니저 스캔이 자동)
//   BindPacketHandlers — 패킷과 처리 멤버 함수 등록: 로그인 BindLoginRequest / 캐시 BindCachedDataRequest
//                        / 조작 BindActionRequest / 노티 BindNotify
//   조작 요청          — SendActionRequest (전송만 — 응답은 BindActionRequest로 등록한 핸들러로 온다)
//   OnReset            — 로그아웃 시 데이터 정리

UCLASS(Abstract)
class PACONTENTSYSTEM_API UPAContentModelBase : public UObject
{
    GENERATED_BODY()

public:
    // 데이터가 필요할 때 조건 없이 호출해도 된다.
    // 각 컨텐츠 모델의 캐시 정책(CachePolicy)에 따라 갱신 요청 발생 여부가 결정된다.
    void RefreshCacheIfNeeded();

    // 캐시를 무효화한다 — 다음 RefreshCacheIfNeeded에서 재요청이 발생함.
    // 컨텐츠 각각 필요한 시점에 호출하도록 한다.
    void InvalidateCache();

    const TArray<TObjectPtr<UPAContentModelBase>>& GetChildren() const { return Children; }

    // 데이터 변경 알림 — 구독자는 알림받으면 모델에서 다시 읽는다
    FSimpleMulticastDelegate OnDataChanged;

protected:
    // 패킷/처리 함수 등록 지점 — 네트워크가 준비된 뒤 베이스가 1회 호출한다
    virtual void BindPacketHandlers() {}
    virtual void OnInitialize() {}
    virtual void OnReset() {}

    // ── 패킷 등록 (BindPacketHandlers에서 호출):
    //    핸들러는 등록한 모델의 멤버 함수. 응답 핸들러는 (성공/실패) 모두 받는다.
    //    실패시 핸들러 호출 전에 Warning 로그 + 캐시 정리를 한다.
    //    전송 시점: 로그인/캐시 데이터는 베이스 소유(HandleLogin/RefreshCacheIfNeeded), 조작은 SendActionRequest.

    // 로그인 시점에 필요한 기본 데이터 요청 등록 (패킷을 분리하여 복수 등록 가능)
    template <typename TRequest, typename UserClass>
    void BindLoginRequest(UserClass* Object,
        void (UserClass::*Func)(EPAContentRequestResult, const typename TRequest::FResponse&))
    {
        RegisterModelResponseHandler<TRequest>(TEXT("Login"), /*bRefresh=*/false, Object, Func);
        LoginRequestSenders.Add([this]()
        {
            SendModelRequest(TEXT("Login"), /*bRefresh=*/false, TRequest());
        });
    }

    // 캐시로 관리되는 데이터 요청 등록 — 전송은 RefreshCacheIfNeeded 때, 대기/유효 상태는 베이스가 관리한다
    template <typename TRequest, typename UserClass>
    void BindCachedDataRequest(UserClass* Object,
        void (UserClass::*Func)(EPAContentRequestResult, const typename TRequest::FResponse&))
    {
        RegisterModelResponseHandler<TRequest>(TEXT("CachedData"), /*bRefresh=*/true, Object, Func);
        CachedDataRequestSender = [this]()
        {
            SendModelRequest(TEXT("CachedData"), /*bRefresh=*/true, TRequest());
        };
    }

    // 유저 조작 요청의 응답 핸들러 등록 — 전송은 SendActionRequest
    template <typename TRequest, typename UserClass>
    void BindActionRequest(UserClass* Object,
        void (UserClass::*Func)(EPAContentRequestResult, const typename TRequest::FResponse&))
    {
        RegisterModelResponseHandler<TRequest>(TEXT("Action"), /*bRefresh=*/false, Object, Func);
    }

    // 노티 핸들러 등록 — 수신 시 처리(캐시 무효화/이벤트 발생 등)
    template <typename TNotify, typename UserClass>
    void BindNotify(UserClass* Object, void (UserClass::*Func)(const TNotify&))
    {
        if (UPAContentNetworkBase* Network = GetNetwork())
        {
            Network->RegisterNotifyHandler<TNotify>(this,
                TDelegate<void(const TNotify&)>::CreateUObject(Object, Func));
        }
    }

    // 유저 조작을 서버에 요청한다. 응답은 BindActionRequest로 등록한 핸들러로 온다.
    template <typename TRequest>
    void SendActionRequest(TRequest Request)
    {
        SendModelRequest(TEXT("Action"), /*bRefresh=*/false, MoveTemp(Request));
    }

    UPAContentModelManagerBase* GetManager() const { return Manager.Get(); }
    UPAContentNetworkBase* GetNetwork() const;
    UPAContentEventDispatcher* GetEventDispatcher() const;

    EPAContentCachePolicy CachePolicy = EPAContentCachePolicy::CacheWithLifetime; // 기본: lifetime = CacheLifetimeSeconds
    double CacheLifetimeSeconds = 30.0;

    // 자식 모델이면 부모 모델 클래스를 지정 (생성자에서). null = 루트.
    // 매니저 초기화 스캔이 이 선언을 읽어 런타임에 부모의 Children에 추가한다.
    TSubclassOf<UPAContentModelBase> ParentModelClass;

private:
    // 수명주기 — friend인 매니저가 호출하고, 자식 전파는 부모 모델이 수행한다
    void HandleInitialize(UPAContentModelManagerBase* InManager);
    void HandleLogin();
    void HandleReset();

    // 매니저에서 스캔 등록과정에서 ParentModelClass에 맞는 자식을 추가한다.
    void AttachChild(UPAContentModelBase* Child);

    // 요청 헬퍼 함수 — 정의는 클래스 아래
    template <typename TRequest>
    void SendModelRequest(const TCHAR* KindForLog, bool bRefresh, TRequest Request);

    template <typename TRequest, typename UserClass>
    void RegisterModelResponseHandler(const TCHAR* KindForLog, bool bRefresh, UserClass* Object,
        void (UserClass::*Func)(EPAContentRequestResult, const typename TRequest::FResponse&));

    // 등록된 응답 핸들러의 공통 앞단 — 캐시/대기 상태 정리(FinishModelRequest) 후 모델 핸들러에 전달한다.
    template <typename TRequest>
    void DispatchModelResponse(EPAContentRequestResult Result, const typename TRequest::FResponse& Response,
        const TCHAR* KindForLog, bool bRefresh,
        TDelegate<void(EPAContentRequestResult, const typename TRequest::FResponse&)> Handler);

    // 템플릿 함수 라인이 길어서 타입 무관 부분을 cpp로 이동한 함수들
    bool BeginModelRequest(const TCHAR* KindForLog, bool bRefresh);  // 네트워크 확인 + 대기 플래그
    void FinishModelRequest(const TCHAR* KindForLog, bool bRefresh, EPAContentRequestResult Result); // 실패 로그 + 캐시 정리

    void MarkRequestSucceeded(); // 캐시 유효 상태 및 대기 해제
    void MarkRequestFailed();    // 대기 해제만 — 재요청 가능 상태로 만들자

    // Bind로 등록된 요청 전송 — HandleLogin/RefreshCacheIfNeeded가 실행한다
    TArray<TFunction<void()>> LoginRequestSenders;
    TFunction<void()> CachedDataRequestSender;

    TWeakObjectPtr<UPAContentModelManagerBase> Manager;

    UPROPERTY()
    TArray<TObjectPtr<UPAContentModelBase>> Children;

    bool bWaitingForResponse = false;
    bool bHasData = false;
    float LastCachedTime = 0.0f;

    // private 수명주기 함수 호출, 스캔 등록 때 ParentModelClass 읽기
    friend class UPAContentModelManagerBase;
};

template <typename TRequest>
void UPAContentModelBase::SendModelRequest(const TCHAR* KindForLog, bool bRefresh, TRequest Request)
{
    if (!BeginModelRequest(KindForLog, bRefresh))
    {
        return;
    }

    GetNetwork()->SendRequest(MoveTemp(Request));
}

template <typename TRequest, typename UserClass>
void UPAContentModelBase::RegisterModelResponseHandler(const TCHAR* KindForLog, bool bRefresh,
    UserClass* Object, void (UserClass::*Func)(EPAContentRequestResult, const typename TRequest::FResponse&))
{
    UPAContentNetworkBase* Network = GetNetwork();
    if (!Network)
    {
        return;
    }

    using FHandlerDelegate = TDelegate<void(EPAContentRequestResult, const typename TRequest::FResponse&)>;
    Network->RegisterResponseHandler<TRequest>(this,
        FHandlerDelegate::CreateUObject(this, &UPAContentModelBase::DispatchModelResponse<TRequest>,
            KindForLog, bRefresh, FHandlerDelegate::CreateUObject(Object, Func)));
}

template <typename TRequest>
void UPAContentModelBase::DispatchModelResponse(EPAContentRequestResult Result, const typename TRequest::FResponse& Response,
    const TCHAR* KindForLog, bool bRefresh,
    TDelegate<void(EPAContentRequestResult, const typename TRequest::FResponse&)> Handler)
{
    FinishModelRequest(KindForLog, bRefresh, Result); // 공통 처리 후 성공/실패 모두 전달
    Handler.ExecuteIfBound(Result, Response);
}
