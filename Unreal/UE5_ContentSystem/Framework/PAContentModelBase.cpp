#include "PAContentModelBase.h"
#include "PAContentModelManagerBase.h"
#include "PAContentSystem.h"

void UPAContentModelBase::HandleInitialize(UPAContentModelManagerBase* InManager)
{
    Manager = InManager;

    if (GetNetwork())
    {
        BindPacketHandlers();
    }
    else
    {
        UE_LOG(LogPAContent, Warning, TEXT("%s: BindPacketHandlers skipped - no network"), *GetClass()->GetName());
    }

    OnInitialize();

    for (UPAContentModelBase* Child : Children)
    {
        Child->HandleInitialize(InManager);
    }
}

void UPAContentModelBase::HandleLogin()
{
    for (const TFunction<void()>& Sender : LoginRequestSenders)
    {
        Sender();
    }
    for (UPAContentModelBase* Child : Children)
    {
        Child->HandleLogin();
    }
}

void UPAContentModelBase::HandleReset()
{
    bWaitingForResponse = false;
    bHasData = false;
    LastCachedTime = 0.0f;

    // 자식 먼저 리셋 — 부모 OnDataChanged 리스너가 자식 데이터를 읽어도 구버전을 보지 않도록
    for (UPAContentModelBase* Child : Children)
    {
        Child->HandleReset();
    }

    OnReset();
    OnDataChanged.Broadcast();
}

void UPAContentModelBase::RefreshCacheIfNeeded()
{
    if (bWaitingForResponse)
    {
        return; // 응답 대기 중 — 중복 요청 방지
    }

    bool bCacheValid = false;
    switch (CachePolicy)
    {
    case EPAContentCachePolicy::CacheUntilInvalidated:
        bCacheValid = bHasData; // InvalidateCache()가 bHasData를 내리기 전까지 유효
        break;
    case EPAContentCachePolicy::CacheWithLifetime:
        bCacheValid = bHasData && (FPlatformTime::Seconds() - LastCachedTime) < CacheLifetimeSeconds;
        break;
    case EPAContentCachePolicy::NoCache:
        bCacheValid = false;
        break;
    }

    if (bCacheValid)
    {
        return;
    }

    // 캐시 데이터 요청을 등록한 컨텐츠 모델
    if (CachedDataRequestSender)
    {
        // bWaitingForResponse 플래그는 전송 시점에 켜진다.
        CachedDataRequestSender();
    }
}

void UPAContentModelBase::InvalidateCache()
{
    bHasData = false;
}

void UPAContentModelBase::AttachChild(UPAContentModelBase* Child)
{
    Children.Add(Child);
}

bool UPAContentModelBase::BeginModelRequest(const TCHAR* KindForLog, bool bRefresh)
{
    if (!GetNetwork())
    {
        UE_LOG(LogPAContent, Warning, TEXT("%s: %s 요청 실패 — 네트워크 없음"), *GetClass()->GetName(), KindForLog);
        return false;
    }

    if (bRefresh)
    {
        bWaitingForResponse = true;
    }
    return true;
}

void UPAContentModelBase::FinishModelRequest(const TCHAR* KindForLog, bool bRefresh, EPAContentRequestResult Result)
{
    if (Result != EPAContentRequestResult::Success)
    {
        UE_LOG(LogPAContent, Warning, TEXT("%s: %s 요청 실패"), *GetClass()->GetName(), KindForLog);
        if (bRefresh)
        {
            MarkRequestFailed();
        }
        return;
    }

    if (bRefresh)
    {
        MarkRequestSucceeded();
    }
}

void UPAContentModelBase::MarkRequestSucceeded()
{
    bWaitingForResponse = false;
    bHasData = true;
    LastCachedTime = static_cast<float>(FPlatformTime::Seconds());
}

void UPAContentModelBase::MarkRequestFailed()
{
    bWaitingForResponse = false;
}

UPAContentNetworkBase* UPAContentModelBase::GetNetwork() const
{
    const UPAContentModelManagerBase* Mgr = Manager.Get();
    return Mgr ? Mgr->GetNetwork() : nullptr;
}

UPAContentEventDispatcher* UPAContentModelBase::GetEventDispatcher() const
{
    const UPAContentModelManagerBase* Mgr = Manager.Get();
    return Mgr ? Mgr->GetEventDispatcher() : nullptr;
}
