#include "PJAbilityDemoCharacter.h"
#include "PJStatComponent.h"
#include "PJStatusEffectComponent.h"
#include "PJAbilityComponent.h"
#include "PJAbilitySyncComponent.h"
#include "PJAbilitySet.h"
#include "PJAbilityData.h"
#include "PJStatusEffectData.h"
#include "PJAbilityTypes.h"
#include "PJTimeSources.h"
#include "Engine/World.h"

APJAbilityDemoCharacter::APJAbilityDemoCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    StatComponent = CreateDefaultSubobject<UPJStatComponent>(TEXT("StatComponent"));
    StatusEffectComponent = CreateDefaultSubobject<UPJStatusEffectComponent>(TEXT("StatusEffectComponent"));
    AbilityComponent = CreateDefaultSubobject<UPJAbilityComponent>(TEXT("AbilityComponent"));
    SyncComponent = CreateDefaultSubobject<UPJAbilitySyncComponent>(TEXT("SyncComponent"));
}

void APJAbilityDemoCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 시간 소스(IPJTimeSource): 네트 모드에 따라 로컬/서버 시계 선택
    UWorld* W = GetWorld();
    if (W && W->GetNetMode() != NM_Standalone)
    {
        TimeSource = MakeShared<FPJServerTimeSource>(W);
    }
    else
    {
        TimeSource = MakeShared<FPJLocalTimeSource>(W);
    }

    // 컴포넌트 간 참조 연결(의존성 주입)
    StatusEffectComponent->SetStatComponent(StatComponent);
    StatusEffectComponent->SetTimeSource(TimeSource);
    AbilityComponent->SetStatComponent(StatComponent);
    AbilityComponent->SetStatusEffectComponent(StatusEffectComponent);
    AbilityComponent->SetTimeSource(TimeSource);
    SyncComponent->SetStatusEffectComponent(StatusEffectComponent);

    // AbilitySet 부여
    if (AbilitySet)
    {
        for (const FPJStatDefinition& Def : AbilitySet->Stats) { StatComponent->AddStat(Def); }
        for (UPJAbilityData* Ab : AbilitySet->Abilities) { AbilityComponent->GrantAbility(Ab); }
        for (UPJStatusEffectData* Fx : AbilitySet->StartupEffects)
        {
            StatusEffectComponent->ApplyEffect(Fx, FPJStatModifierHandle::Generate(),
                TimeSource->GetTimeSeconds());
        }
    }
}

void APJAbilityDemoCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    StatusEffectComponent->TickEffects();
    AbilityComponent->TickAbilities();
}
