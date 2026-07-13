#include "PAGameContentModelManager.h"
#include "PAGameContentNetwork.h"

void UPAGameContentModelManager::CreateNetwork()
{
    SetNetwork(NewObject<UPAGameContentNetwork>(this));
}
