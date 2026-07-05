#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "PJAbilitySubsystem.generated.h"

class UPJAbilityData;
class UPJStatusEffectData;

/**
 * 어빌리티 서브시스템 - 어빌리티/효과 정의를 한곳에 등록해 두고 태그로 찾게 해주는 중앙 창구.
 * 데이터 주도 구조: 컴포넌트는 태그로 조회만 하고, 정의의 소유와 수명은 여기서 관리한다.
 * 동기화 계층이 스냅샷을 다시 맞출 때 태그로 원본 데이터에셋을 되찾는 데도 쓴다(PJAbilitySyncComponent).
 */
UCLASS()
class PJABILITYSYSTEM_API UPJAbilitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void RegisterAbility(UPJAbilityData* Data);
    void RegisterEffect(UPJStatusEffectData* Data);

    UPJAbilityData* FindAbilityByTag(FGameplayTag AbilityTag) const;
    UPJStatusEffectData* FindEffectByTag(FGameplayTag EffectTag) const;

private:
    UPROPERTY() TMap<FGameplayTag, TObjectPtr<UPJAbilityData>> AbilityRegistry;
    UPROPERTY() TMap<FGameplayTag, TObjectPtr<UPJStatusEffectData>> EffectRegistry;
};
