#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentMessage.h"
#include "MailContentMessages.generated.h"

// UI/BP에 노출되는 메일 데이터
USTRUCT(BlueprintType)
struct FMailData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName MailId;

    UPROPERTY(BlueprintReadOnly)
    FText Title;

    UPROPERTY(BlueprintReadOnly)
    FText Body;

    UPROPERTY(BlueprintReadOnly)
    bool bRead = false;

    friend FArchive& operator<<(FArchive& Ar, FMailData& Data)
    {
        Ar << Data.MailId;
        Ar << Data.Title;
        Ar << Data.Body;
        Ar << Data.bRead;
        return Ar;
    }
};

// ── 메일 네트워크 메시지──────────────
// 필드 추가 시 Serialize 갱신 필수

// LoginRequest : 로그인 시 메일 요약 정보 요청 응답
struct FMailSummaryResponse : public FPAContentMessage
{
    int32 UnreadCount = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.Summary")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << UnreadCount; }
};
// LoginRequest : 로그인 시 메일 요약 정보 요청
struct FMailSummaryRequest : public TPAContentRequest<FMailSummaryResponse>
{
    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.Summary")); return Id; }
    virtual void Serialize(FArchive& Ar) override {}
};

// CachedDataRequest : 목록 상세 정보 요청 응답
struct FMailListResponse : public FPAContentMessage
{
    TArray<FMailData> Mails;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << Mails; }
};
// CachedDataRequest : 목록 상세 정보 요청 
struct FMailListRequest : public TPAContentRequest<FMailListResponse>
{
    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override {}
};

// ActionRequest : 메일 읽음 요청 응답
struct FMailReadResponse : public FPAContentMessage
{
    bool bSuccess = false;
    FName MailId;
    int32 UnreadCount = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.Read")); return Id; }
    virtual void Serialize(FArchive& Ar) override
    {
        Ar << bSuccess;
        Ar << MailId;
        Ar << UnreadCount;
    }
};

// ActionRequest : 메일 읽음 요청
struct FMailReadRequest : public TPAContentRequest<FMailReadResponse>
{
    FName MailId;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.Read")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << MailId; }
};

// 새 메일 도착 노티. InvalidateCache로 캐시를 무효화 시키고 RefreshCacheIfNeeded 시점에 캐시를 갱신한다.
struct FMailNewMailNotify : public FPAContentMessage
{
    int32 UnreadCount = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Mail.NewMail")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << UnreadCount; }
};
