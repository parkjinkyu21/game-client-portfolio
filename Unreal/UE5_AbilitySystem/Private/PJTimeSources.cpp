#include "PJTimeSources.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

FPJLocalTimeSource::FPJLocalTimeSource(UWorld* InWorld) : World(InWorld) {}

double FPJLocalTimeSource::GetTimeSeconds() const
{
    return World.IsValid() ? World->GetTimeSeconds() : 0.0;
}

FPJServerTimeSource::FPJServerTimeSource(UWorld* InWorld) : World(InWorld) {}

double FPJServerTimeSource::GetTimeSeconds() const
{
    if (World.IsValid())
    {
        if (AGameStateBase* GS = World->GetGameState())
        {
            return GS->GetServerWorldTimeSeconds();
        }
        return World->GetTimeSeconds();
    }
    return 0.0;
}
