// AI Character Companion Application
// Built with Unreal Engine 5

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "PJSkeletalMeshComponent.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(PJSkeletalMeshComponent, Log, All);

struct FPJClothMorphData;

/** 커스텀 머티리얼 슬롯 데이터 (동적 머티리얼 인스턴스 및 파라미터 관리) */
USTRUCT()
struct FCustomMaterialData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int MaterialSlotIndex = 0;

	TArray<FCVMaterialDatas> MaterialParamDatas;

	UPROPERTY()
	TWeakObjectPtr<class UMaterialInstance> OriginalMaterial;

	UPROPERTY()
	TWeakObjectPtr<class UMaterialInstanceDynamic> InstanceMaterial;
};

/**
 * 캐릭터 커스터마이징을 위한 확장 스켈레탈 메시 컴포넌트
 *
 * - Leader/Follower Pose 기반 스켈레톤 공유
 * - 비동기 메시/애니메이션/머티리얼 로딩 및 콜백 처리
 * - 클론 메시 생성 및 모프 타겟 전파
 * - 동적 머티리얼 파라미터 관리
 * - 메시 슬라이싱 데이터 관리
 */
UCLASS()
class PJCORE_API UPJSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	// === Leader/Follower Pose ===
	void SetIgnoreMasterPose(bool bIgnore);
	virtual void SetMasterComponent(USkeletalMeshComponent* Comp, bool bForceUpdate = false, bool bInFollowerShouldTickPose = false);
	void UpdateIgnoreMasterPose();
	virtual void UpdateFollowerComponent() override;

	UFUNCTION(BlueprintCallable, Category = "CVSkeletalMesh")
	virtual void ChangeIgnoreMasterpose(bool bOut);

	// === 메시 로딩 ===
	void LoadMesh(FSoftObjectPath ToLoad, int32 ItemID);
	void RemoveMesh();
	void SyncLoadMeshAnim(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadAnim);
	void AsyncLoadMeshAnim(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadAnim);
	void AsyncLoadMeshMaterial(FSoftObjectPath ToLoadMesh, FSoftObjectPath ToLoadMat, FString SlotName, int32 ItemID = -1);
	void AsyncLoadAnim(FSoftObjectPath ToLoadAnim);

	virtual void SetSkeletalMesh(class USkeletalMesh* NewMesh, bool bReinitPose = true) override;
	virtual void SetPhysicsAsset(class UPhysicsAsset* NewPhysicsAsset, bool bForceReInit = false) override;

	void SetOriginalSkeletalMesh(USkeletalMesh* NewMesh);
	USkeletalMesh* GetSkeletalMesh();
	USkeletalMesh* GetOriginalSkeletalMesh();

	// === 클론 메시 ===
	void MakeCloneSkeletalMesh(TArray<TSharedPtr<class FSliceConditionBase>>& OutSlicerArray, class UPhysicsAsset* Physics = nullptr);
	void AddCloneMeshComponent(UPJSkeletalMeshComponent* Clone, bool bUpdateLocation = true);
	void RemoveCloneMeshComponent(UPJSkeletalMeshComponent* Clone);

	// === 모프 타겟 ===
	void ClearMorphTargetsWithChild();
	void SetMorphTargetWithChild(FName MorphTargetName, float Value, bool bRemoveZeroWeight = true);
	void SetClothMorphTarget(FString SlotPrefix, const FPJClothMorphData& MorphData);

	// === 머티리얼 ===
	void LoadMaterial(FSoftObjectPath ToLoad, FString SlotName);
	class UMaterialInstanceDynamic* FindCustomMaterial(FString Name);
	void ChangeMaterialData(FCVMaterialDatas& Data, bool bApplyTexture = true);
	void ResetMaterialData(bool bForce = true);
	void CopyMaterialFromComponent(UPJSkeletalMeshComponent* Original);
	void MirroringMaterialFromComponent(UPJSkeletalMeshComponent* Original);
	TArray<FCustomMaterialData>& GetCustomMaterialDatas() { return CustomMaterialDatas; }

	// === 슬라이서 데이터 ===
	void ResetSlicerData();
	void SetSlicerData(FString& Name);
	FString& GetSlicerName() { return MySlicerName; }

	// === 소켓 어태치 ===
	UFUNCTION(BlueprintCallable, Category = "CVSkeletalMesh")
	virtual void SetAttachSocketName(FName InSocketName);

	UFUNCTION(BlueprintCallable, Category = "CVSkeletalMesh")
	void UpdateAttachSocketName();

	// === 유틸리티 ===
	void ResetDatas();

	UFUNCTION(BlueprintCallable, Category = "CVSkeletalMesh")
	void Overlaped(bool bSelect, int32 Color);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// === 비동기 로딩 콜백 ===
	void OnLoadCompleted(UObject* Loaded, int32 ItemID);
	void OnLoadCompleteTextureParam(UObject* Loaded, TWeakObjectPtr<UMaterialInstanceDynamic> Material, FString ParamName);
	void OnLoadCompleteMaterial(UObject* Loaded, FString SlotName);
	void OnLoadCompleteAnim(UObject* Loaded);
	void OnLoadCompleteMeshAnim(TArray<UObject*> Loaded, TArray<FSoftObjectPath> Paths);
	void OnLoadCompleteMeshMaterial(TArray<UObject*> Loaded, FString SlotName, TArray<FSoftObjectPath> Paths, int32 ItemID);

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> MasterComponent;

	// 클론 생성을 위한 원본 메시 보관
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> OriginalSkeletalMesh;

	UPROPERTY(Transient)
	TArray<FCustomMaterialData> CustomMaterialDatas;

	// 메시 로딩 완료 전 머티리얼이 먼저 도착한 경우 지연 적용
	UPROPERTY(Transient)
	UMaterialInstance* DeferredMaterial = nullptr;

	UPROPERTY(Transient)
	TArray<FCVMaterialDatas> DeferredMaterialData;

	FString DeferredSlotName;
	bool bHasDeferredMaterial = false;

	FString MySlicerName;

	UPROPERTY(Transient)
	TArray<UPJSkeletalMeshComponent*> CloneMeshComponents;

public:
	// 스켈레톤을 공유하지 않는 경우 링크될 소켓 이름
	UPROPERTY(EditDefaultsOnly, Category = "CVSkeletalMesh", Meta = (AllowPrivateAccess = true))
	FName SocketName;

	UPROPERTY(EditDefaultsOnly, Category = "CVSkeletalMesh", Meta = (AllowPrivateAccess = true))
	FName VirtualPartsSocketName;

	// true: LeaderPose 사용 안 함, 자체 애니메이션 사용 (머리카락, 가방 등)
	UPROPERTY(EditDefaultsOnly, Category = "CVSkeletalMesh", Meta = (AllowPrivateAccess = true))
	bool IgnoreMasterpose = false;
};
