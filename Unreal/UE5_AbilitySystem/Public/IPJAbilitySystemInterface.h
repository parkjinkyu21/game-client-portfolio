#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPJAbilitySystemInterface.generated.h"

class UPJStatComponent;
class UPJAbilityComponent;
class UPJStatusEffectComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UPJAbilitySystemInterface : public UInterface { GENERATED_BODY() };

/**
 * 오너 추상화 인터페이스 - 특정 Pawn 타입 하드 의존을 제거(드롭인 친화).
 * 어떤 액터든 이 인터페이스만 구현하면 PJ 컴포넌트에 접근할 수 있다.
 * (기본은 컴포넌트-온-액터, 폰 교체 지속성이 필요하면 PlayerState에 배치하는 방식도 가능)
 */
class PJABILITYSYSTEM_API IPJAbilitySystemInterface
{
    GENERATED_BODY()
public:
    virtual UPJStatComponent* GetPJStatComponent() const = 0;
    virtual UPJAbilityComponent* GetPJAbilityComponent() const = 0;
    virtual UPJStatusEffectComponent* GetPJStatusEffectComponent() const = 0;
};
