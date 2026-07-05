#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PJAbilityTypes.h"
#include "IPJTimeSource.h"
#include "PJStatusEffect.generated.h"

class UPJStatusEffectData;

/**
 * 걸려 있는 상태효과 하나를 나타내는 런타임 객체.
 *
 * [타이밍 - 서버 시간 앵커로 클라 간 만료 시점 일치]
 * 로컬 경과시간(elapsed += dt)을 쓰지 않는다. StartTime + Duration을 보관하고
 * 남은시간 = Duration - (now - StartTime)로 계산한다(now는 IPJTimeSource).
 * 덕분에 프레임레이트나 이벤트 지연, 접속 시점이 달라도 모든 클라가 같은 만료 시점에 수렴한다.
 *
 * [주기 적용] ConsumeDuePeriods(): dt 누적 없이 floor((now-Start)/Period) 경계로 경과 횟수만 반환.
 * [스택] Refresh(StartTime 갱신) / AddStack(중첩, MaxStacks 한도).
 */
UCLASS()
class PJABILITYSYSTEM_API UPJStatusEffect : public UObject
{
    GENERATED_BODY()
public:
    void Init(UPJStatusEffectData* InData, FPJStatModifierHandle InSource,
              double InStartTime, TSharedPtr<IPJTimeSource> InTimeSource);

    UPJStatusEffectData* GetData() const { return Data; }
    FPJStatModifierHandle GetSource() const { return Source; }
    double GetStartTime() const { return StartTime; }
    int32 GetStackCount() const { return StackCount; }

    double GetRemaining() const;   // Infinite면 큰 양수, Instant면 0
    bool IsExpired() const;        // Duration이고 Remaining<=0
    void Refresh(double NewStartTime);
    void AddStack();

    // dt 누적 없이 서버 시간 경계로 경과 주기 수 계산 후 소비. 비주기면 0.
    int32 ConsumeDuePeriods();

private:
    UPROPERTY() TObjectPtr<UPJStatusEffectData> Data = nullptr;
    FPJStatModifierHandle Source;
    double StartTime = 0.0;
    int32 StackCount = 1;
    int32 PeriodsApplied = 0;
    TSharedPtr<IPJTimeSource> TimeSource;

    double Now() const;
};
