// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PJCostumeInterface.generated.h"


UINTERFACE(MinimalAPI)
class UPJCostumeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 코스튬 비동기 로딩 콜백 인터페이스
 *
 * 스켈레탈/스태틱 메시, 애니메이션, 머티리얼의 비동기 로딩 완료 시점을
 * 캐릭터 클래스에 통지하기 위한 인터페이스.
 * APJCostumeCharacter가 이를 구현하여 로딩 완료 후 후처리(장착, LOD 동기화 등)를 수행.
 */
class PJCORE_API IPJCostumeInterface
{
	GENERATED_BODY()

public:
	virtual void OnSkeletalLoadCompleted(class UPJSkeletalMeshComponent* Comp, int32 ItemID) = 0;

	virtual void OnAnimLoadCompleted(class UPJSkeletalMeshComponent* Comp)  = 0;

	virtual void OnPreLoadSkeletalMesh(class UPJSkeletalMeshComponent* Comp) = 0;

	virtual void OnStaticLoadCompleted(class UPJStaticMeshComponent* Comp) = 0;

	virtual void OnPreLoadStaticMesh(class UPJStaticMeshComponent* Comp) = 0;

	virtual void OnMaterialLoadComplete(class UPJSkeletalMeshComponent* Comp) = 0;
};
