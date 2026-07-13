#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PAContentEventDispatcher.h"
#include "PAContentModelManagerBase.generated.h"

class UPAContentModelBase;
class UPAContentNetworkBase;

// 매니저 베이스: 등록/조회/수명주기만. 모델 특화 로직 금지.
// 모델의 BP 파생 클래스는 스캔되지 않고 네이티브 클래스만 스캔하여 ModelMap에 등록된다.
UCLASS(Abstract)
class PACONTENTSYSTEM_API UPAContentModelManagerBase : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    template <typename T>
    T* GetModel() const
    {
        return Cast<T>(ModelMap.FindRef(T::StaticClass()));
    }

    UPAContentModelBase* GetModelByClass(TSubclassOf<UPAContentModelBase> ModelClass) const;

    // 월드 컨텍스트에서 서브시스템 조회. null 반환 가능.
    template <typename T>
    static T* FindSubsystem(const UObject* WorldContextObject)
    {
        UGameInstance* GameInstance = ResolveGameInstance(WorldContextObject);
        return GameInstance ? GameInstance->GetSubsystem<T>() : nullptr;
    }

    // 로그인 → 모든 모델의 등록된 로그인 요청 전송 → OnLoginCompleted 발화 (로딩 해제/인게임 진입 지점)
    void StartLogin();
    void ResetAll();

    UPAContentNetworkBase* GetNetwork() const { return Network; }
    UPAContentEventDispatcher* GetEventDispatcher() const { return EventDispatcher; }

    FSimpleMulticastDelegate OnLoginCompleted;

    // ── 컨텐츠 이벤트 static 헬퍼 ─────────────────────────────────────────
    // 월드 컨텍스트에서 매니저/디스패처를 해석해 호출을 대행한다.
    // 등록/발행 실패는 Error 로그를 남기고 넘어간다. 해제 실패는 로그 없이 무시한다.

    template <typename UserClass, typename TEvent>
    static FDelegateHandle RegisterContentEvent(const UObject* WorldContextObject,
        FGameplayTag Channel, UserClass* Object, void (UserClass::*Func)(const TEvent&))
    {
        UPAContentEventDispatcher* Dispatcher = ResolveDispatcher(WorldContextObject, TEXT("RegisterContentEvent"));
        return Dispatcher
            ? Dispatcher->RegisterListener<TEvent>(Channel, TDelegate<void(const TEvent&)>::CreateUObject(Object, Func))
            : FDelegateHandle();
    }

    static void UnregisterContentEvent(const UObject* WorldContextObject,
        FGameplayTag Channel, FDelegateHandle Handle);

    template <typename TEvent>
    static void BroadcastContentEvent(const UObject* WorldContextObject,
        FGameplayTag Channel, const TEvent& Event)
    {
        if (UPAContentEventDispatcher* Dispatcher = ResolveDispatcher(WorldContextObject, TEXT("BroadcastContentEvent")))
        {
            Dispatcher->Broadcast(Channel, Event);
        }
    }

protected:
    virtual void CreateNetwork() {}

    void SetNetwork(UPAContentNetworkBase* InNetwork) { Network = InNetwork; }

private:
    // 월드 컨텍스트 → 게임 인스턴스 해석 (FindSubsystem/ResolveDispatcher 공용). null 반환 가능.
    static UGameInstance* ResolveGameInstance(const UObject* WorldContextObject);

    // ErrorContext가 null이면 실패해도 로그 없이 null 반환 (해제 경로용)
    static UPAContentEventDispatcher* ResolveDispatcher(const UObject* WorldContextObject, const TCHAR* ErrorContext);

    void RegisterModelsByScan();
    void AddToMap(UPAContentModelBase* Model);

    UPROPERTY()
    TMap<TObjectPtr<UClass>, TObjectPtr<UPAContentModelBase>> ModelMap;

    UPROPERTY()
    TArray<TObjectPtr<UPAContentModelBase>> RootModels;

    UPROPERTY()
    TObjectPtr<UPAContentNetworkBase> Network;

    UPROPERTY()
    TObjectPtr<UPAContentEventDispatcher> EventDispatcher;

};
