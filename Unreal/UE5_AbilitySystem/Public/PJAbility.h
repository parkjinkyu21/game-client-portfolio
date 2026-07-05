#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "PJAbility.generated.h"

class UPJAbilityData;
class UPJStatComponent;

/**
 * 어빌리티 런타임 인스턴스 - 발동 가능 판정과 활성 상태를 담는다.
 * 코스트 차감/효과 적용 등 부수효과는 UPJAbilityComponent가 오케스트레이션하고,
 * 이 클래스는 CanActivate(요건/차단/쿨다운/코스트 판정)와 활성 플래그만 책임진다.
 */
UCLASS()
class PJABILITYSYSTEM_API UPJAbility : public UObject
{
    GENERATED_BODY()
public:
    void Init(UPJAbilityData* InData);
    UPJAbilityData* GetData() const { return Data; }

    bool CanActivate(const FGameplayTagContainer& OwnerTags, UPJStatComponent* Stat) const;
    bool Activate();
    void Deactivate();
    bool IsActive() const { return bActive; }
    bool WantsTick() const;

protected:
    UPROPERTY() TObjectPtr<UPJAbilityData> Data = nullptr;
    bool bActive = false;
};
