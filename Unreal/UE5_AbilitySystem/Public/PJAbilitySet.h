#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PJStatData.h"
#include "PJAbilitySet.generated.h"

class UPJAbilityData;
class UPJStatusEffectData;

/**
 * 어빌리티 세트 - 캐릭터에 한 번에 부여할 스탯+어빌리티+시작효과 번들.
 * 데모 캐릭터가 BeginPlay에서 이 세트를 읽어 컴포넌트에 일괄 부여한다.
 */
UCLASS(BlueprintType)
class PJABILITYSYSTEM_API UPJAbilitySet : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) TArray<FPJStatDefinition> Stats;
    UPROPERTY(EditDefaultsOnly) TArray<TObjectPtr<UPJAbilityData>> Abilities;
    UPROPERTY(EditDefaultsOnly) TArray<TObjectPtr<UPJStatusEffectData>> StartupEffects;
};
