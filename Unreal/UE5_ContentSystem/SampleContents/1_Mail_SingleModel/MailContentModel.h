#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentModelBase.h"
#include "MailContentMessages.h"
#include "MailContentModel.generated.h"

// 메일 컨텐츠 모델.
// 메일 목록 캐시와 읽음 처리, 새 메일 노티 수신을 담당한다.
UCLASS()
class PACONTENTSYSTEM_API UMailContentModel : public UPAContentModelBase
{
    GENERATED_BODY()

public:
    UMailContentModel();

    const TArray<FMailData>& GetMails() const { return Mails; }
    void RequestRead(FName MailId);

protected:
    virtual void BindPacketHandlers() override;
    virtual void OnReset() override;

private:
    void OnSummaryResponse(EPAContentRequestResult Result, const FMailSummaryResponse& Response);
    void OnListResponse(EPAContentRequestResult Result, const FMailListResponse& Response);
    void OnReadResponse(EPAContentRequestResult Result, const FMailReadResponse& Response);
    void OnNewMailNotify(const FMailNewMailNotify& Notify);

    void BroadcastUnreadCount(int32 UnreadCount);

    UPROPERTY()
    TArray<FMailData> Mails;
};
