#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentMessage.h"
#include "CollectionContentMessages.generated.h"

// 타이틀 등급
UENUM(BlueprintType)
enum class ECollectionTitleGrade : uint8
{
    Normal,
    Rare,
    Legacy,
    Unique,
};

// UI/BP에 노출되는 외형 도감 항목
USTRUCT(BlueprintType)
struct FAppearanceEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName AppearanceId;

    UPROPERTY(BlueprintReadOnly)
    FText Name;

    UPROPERTY(BlueprintReadOnly)
    bool bCollected = false;

    friend FArchive& operator<<(FArchive& Ar, FAppearanceEntry& Entry)
    {
        Ar << Entry.AppearanceId;
        Ar << Entry.Name;
        Ar << Entry.bCollected;
        return Ar;
    }
};

// UI/BP에 노출되는 타이틀 항목
USTRUCT(BlueprintType)
struct FTitleEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName TitleId;

    UPROPERTY(BlueprintReadOnly)
    FText Name;

    UPROPERTY(BlueprintReadOnly)
    ECollectionTitleGrade Grade = ECollectionTitleGrade::Normal;

    UPROPERTY(BlueprintReadOnly)
    bool bOwned = false;

    UPROPERTY(BlueprintReadOnly)
    bool bEquipped = false;

    friend FArchive& operator<<(FArchive& Ar, FTitleEntry& Entry)
    {
        Ar << Entry.TitleId;
        Ar << Entry.Name;
        uint8 GradeValue = static_cast<uint8>(Entry.Grade);
        Ar << GradeValue;
        Entry.Grade = static_cast<ECollectionTitleGrade>(GradeValue);
        Ar << Entry.bOwned;
        Ar << Entry.bEquipped;
        return Ar;
    }
};

// ── 컬렉션 네트워크 메시지──────────────
// 필드 추가 시 Serialize 갱신 필수

// LoginRequest : 로그인 시 수집 진행 요약 요청 응답
struct FCollectionSummaryResponse : public FPAContentMessage
{
    int32 AppearanceCollected = 0;
    int32 AppearanceTotal = 0;
    int32 TitleOwned = 0;
    int32 TitleTotal = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Summary")); return Id; }
    virtual void Serialize(FArchive& Ar) override
    {
        Ar << AppearanceCollected;
        Ar << AppearanceTotal;
        Ar << TitleOwned;
        Ar << TitleTotal;
    }
};
// LoginRequest : 로그인 시 수집 진행 요약 요청
struct FCollectionSummaryRequest : public TPAContentRequest<FCollectionSummaryResponse>
{
    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Summary")); return Id; }
    virtual void Serialize(FArchive& Ar) override {}
};

// CachedDataRequest : 외형 도감 목록 요청 응답
struct FAppearanceListResponse : public FPAContentMessage
{
    TArray<FAppearanceEntry> Appearances;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Appearance.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << Appearances; }
};
// CachedDataRequest : 외형 도감 목록 요청 — RefreshCacheIfNeeded
struct FAppearanceListRequest : public TPAContentRequest<FAppearanceListResponse>
{
    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Appearance.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override {}
};

// ActionRequest : 외형 등록 요청 응답
struct FAppearanceRegisterResponse : public FPAContentMessage
{
    bool bSuccess = false;
    FName AppearanceId;
    int32 CollectedCount = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Appearance.Register")); return Id; }
    virtual void Serialize(FArchive& Ar) override
    {
        Ar << bSuccess;
        Ar << AppearanceId;
        Ar << CollectedCount;
    }
};
// ActionRequest : 외형 등록 요청
struct FAppearanceRegisterRequest : public TPAContentRequest<FAppearanceRegisterResponse>
{
    FName AppearanceId;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Appearance.Register")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << AppearanceId; }
};

// CachedDataRequest : 타이틀 목록 요청 응답
struct FTitleListResponse : public FPAContentMessage
{
    TArray<FTitleEntry> Titles;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Title.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << Titles; }
};
// CachedDataRequest : 타이틀 목록 요청 — RefreshCacheIfNeeded
struct FTitleListRequest : public TPAContentRequest<FTitleListResponse>
{
    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Title.List")); return Id; }
    virtual void Serialize(FArchive& Ar) override {}
};

// ActionRequest : 타이틀 장착 요청 응답
struct FTitleEquipResponse : public FPAContentMessage
{
    bool bSuccess = false;
    FName TitleId;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Title.Equip")); return Id; }
    virtual void Serialize(FArchive& Ar) override
    {
        Ar << bSuccess;
        Ar << TitleId;
    }
};
// ActionRequest : 타이틀 장착 요청
struct FTitleEquipRequest : public TPAContentRequest<FTitleEquipResponse>
{
    FName TitleId;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Title.Equip")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << TitleId; }
};

// 새 타이틀 획득 노티.
struct FTitleAcquiredNotify : public FPAContentMessage
{
    FName TitleId;
    int32 OwnedCount = 0;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Title.Acquired")); return Id; }
    virtual void Serialize(FArchive& Ar) override
    {
        Ar << TitleId;
        Ar << OwnedCount;
    }
};

// 외형 획득 노티.
struct FAppearanceAcquiredNotify : public FPAContentMessage
{
    FName AppearanceId;

    virtual FName GetMessageId() const override { static const FName Id(TEXT("Collection.Appearance.Acquired")); return Id; }
    virtual void Serialize(FArchive& Ar) override { Ar << AppearanceId; }
};
