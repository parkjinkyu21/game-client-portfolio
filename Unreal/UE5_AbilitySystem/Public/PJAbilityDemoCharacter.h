#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IPJAbilitySystemInterface.h"
#include "IPJTimeSource.h"
#include "PJAbilityDemoCharacter.generated.h"

class UPJStatComponent;
class UPJAbilityComponent;
class UPJStatusEffectComponent;
class UPJAbilitySyncComponent;
class UPJAbilitySet;

// PJ 어빌리티 시스템 데모/샘플 캐릭터.
// 스탯/상태효과/어빌리티/동기화 컴포넌트를 부착하고 IPJAbilitySystemInterface를 구현한다.
// BeginPlay에서 NetMode에 맞춰 IPJTimeSource를 주입하고 AbilitySet을 부여하며,
// Tick에서 상태효과/어빌리티를 갱신한다. (드롭인 사용 예시)
UCLASS()
class PJABILITYSYSTEM_API APJAbilityDemoCharacter : public ACharacter, public IPJAbilitySystemInterface
{
    GENERATED_BODY()
public:
    APJAbilityDemoCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual UPJStatComponent* GetPJStatComponent() const override { return StatComponent; }
    virtual UPJAbilityComponent* GetPJAbilityComponent() const override { return AbilityComponent; }
    virtual UPJStatusEffectComponent* GetPJStatusEffectComponent() const override { return StatusEffectComponent; }

    UPROPERTY(EditDefaultsOnly, Category="PJ") TObjectPtr<UPJAbilitySet> AbilitySet = nullptr;

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPJStatComponent> StatComponent;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPJStatusEffectComponent> StatusEffectComponent;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPJAbilityComponent> AbilityComponent;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPJAbilitySyncComponent> SyncComponent;

    TSharedPtr<IPJTimeSource> TimeSource;
};
