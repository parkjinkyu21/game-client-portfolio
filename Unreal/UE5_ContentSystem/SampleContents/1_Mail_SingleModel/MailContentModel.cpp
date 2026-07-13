#include "MailContentModel.h"
#include "Framework/PAContentModelManagerBase.h"
#include "MailContentEvents.h"

UMailContentModel::UMailContentModel()
{
    // 새 메일 노티가 캐시를 무효화하므로 수명 없이 무효화 기반으로 유지한다.
    // 대안: 첫 열람 때 한 번 받아 두고 이후 동기화 패킷으로 캐시를 직접 수정하는 방식
    CachePolicy = EPAContentCachePolicy::CacheUntilInvalidated;
}

void UMailContentModel::BindPacketHandlers()
{
    BindLoginRequest<FMailSummaryRequest>(this, &UMailContentModel::OnSummaryResponse);
    BindCachedDataRequest<FMailListRequest>(this, &UMailContentModel::OnListResponse);
    BindActionRequest<FMailReadRequest>(this, &UMailContentModel::OnReadResponse);
    BindNotify<FMailNewMailNotify>(this, &UMailContentModel::OnNewMailNotify);
}

void UMailContentModel::RequestRead(FName MailId)
{
    FMailReadRequest Request;
    Request.MailId = MailId;
    SendActionRequest(MoveTemp(Request));
}

// 로그인 요청 응답: 요약(안읽은 수) → Mail_Event_UnreadCount 이벤트 발생
void UMailContentModel::OnSummaryResponse(EPAContentRequestResult Result, const FMailSummaryResponse& Response)
{
    if (Result != EPAContentRequestResult::Success)
    {
        return; // 실패해도 무시 — 안읽은 수 이벤트는 다음 목록 갱신에서 다시 발생한다
    }

    BroadcastUnreadCount(Response.UnreadCount);
}

// 캐시 데이터 요청 응답: 메일 목록
void UMailContentModel::OnListResponse(EPAContentRequestResult Result, const FMailListResponse& Response)
{
    if (Result != EPAContentRequestResult::Success)
    {
        return;
    }

    Mails = Response.Mails;

    int32 Unread = 0;
    for (const FMailData& Mail : Mails)
    {
        Unread += Mail.bRead ? 0 : 1;
    }
    BroadcastUnreadCount(Unread);
    OnDataChanged.Broadcast();
}

// 조작 요청 응답: 읽음 처리 — 모델 데이터에 반영
void UMailContentModel::OnReadResponse(EPAContentRequestResult Result, const FMailReadResponse& Response)
{
    if (Result != EPAContentRequestResult::Success || !Response.bSuccess)
    {
        return;
    }

    for (FMailData& Mail : Mails)
    {
        if (Mail.MailId == Response.MailId)
        {
            Mail.bRead = true;
            break;
        }
    }
    BroadcastUnreadCount(Response.UnreadCount);
    OnDataChanged.Broadcast();
}

// 새 메일 노티: 목록 캐시 무효화 + Mail_Event_UnreadCount 이벤트 발생
void UMailContentModel::OnNewMailNotify(const FMailNewMailNotify& Notify)
{
    InvalidateCache(); // 목록은 다음 열람 때 재요청
    BroadcastUnreadCount(Notify.UnreadCount);
}

void UMailContentModel::OnReset()
{
    Mails.Reset();
}

void UMailContentModel::BroadcastUnreadCount(int32 UnreadCount)
{
    UPAContentModelManagerBase::BroadcastContentEvent(this, PAContentEvents::Mail_Event_UnreadCount,
        FMailUnreadCountChanged(UnreadCount));
}
