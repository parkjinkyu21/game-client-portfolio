#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentModelManagerBase.h"
#include "PAGameContentModelManager.generated.h"

// 게임 컨텐츠 매니저 클래스.
// 지금은 게임 컨텐츠에 맞는 Network 클래스만 등록한다.
UCLASS()
class PACONTENTSYSTEM_API UPAGameContentModelManager : public UPAContentModelManagerBase
{
    GENERATED_BODY()

public:
    static UPAGameContentModelManager* Get(const UObject* WorldContextObject)
    {
        return FindSubsystem<UPAGameContentModelManager>(WorldContextObject);
    }

protected:
    virtual void CreateNetwork() override;
};
