#include "PAContentModelManagerBase.h"
#include "PAContentModelBase.h"
#include "PAContentNetworkBase.h"
#include "PAContentSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UObjectHash.h"

void UPAContentModelManagerBase::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    EventDispatcher = NewObject<UPAContentEventDispatcher>(this);
    CreateNetwork(); // 파생(게임)이 게임 네트워크를 생성해 연결한다

    RegisterModelsByScan();
}

void UPAContentModelManagerBase::Deinitialize()
{
    ModelMap.Reset();
    RootModels.Reset();
    Network = nullptr;
    EventDispatcher = nullptr;
    Super::Deinitialize();
}

UPAContentModelBase* UPAContentModelManagerBase::GetModelByClass(TSubclassOf<UPAContentModelBase> ModelClass) const
{
    return ModelMap.FindRef(ModelClass);
}

void UPAContentModelManagerBase::RegisterModelsByScan()
{
    // 파생 클래스 수집 — 네이티브 구체 클래스만 (BP 파생 미지원. SKEL_/REINST_도 비네이티브라 함께 걸러진다)
    // 등록 순서는 계약이 아니다 — 모델 간 순서 의존은 설계상 금지 (필요해지면 DependsOn 선언으로 드러낸다)
    TArray<UClass*> Classes;
    GetDerivedClasses(UPAContentModelBase::StaticClass(), Classes, true);
    Classes.RemoveAll([](const UClass* C)
    {
        return !C || !C->HasAnyClassFlags(CLASS_Native) || C->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated);
    });

    // 검증 + 부모 해석. 계약: 컨텐츠 모델은 리프 클래스, 계층은 루트-자식 2단까지
    TMap<UClass*, UClass*> ParentMap; // child → parent (null = 루트)
    for (UClass* C : Classes)
    {
        // 리프 관례: 스캔된 다른 구체 모델을 상속하면 이중 인스턴스가 생긴다
        const bool bInheritsConcreteModel = Classes.ContainsByPredicate(
            [C](const UClass* Other) { return Other != C && C->IsChildOf(Other); });
        if (bInheritsConcreteModel)
        {
            UE_LOG(LogPAContent, Error, TEXT("RegisterModelsByScan: %s — 스캔 대상 구체 모델을 상속함 (리프 클래스 관례 위반) — 스킵"), *C->GetName());
            continue;
        }

        UClass* ParentClass = GetDefault<UPAContentModelBase>(C)->ParentModelClass.Get();
        if (ParentClass)
        {
            if (ParentClass->HasAnyClassFlags(CLASS_Abstract) || !Classes.Contains(ParentClass))
            {
                UE_LOG(LogPAContent, Error, TEXT("RegisterModelsByScan: %s의 ParentModelClass(%s)가 Abstract이거나 스캔 목록에 없음 — 스킵"),
                    *C->GetName(), *ParentClass->GetName());
                continue;
            }
            if (GetDefault<UPAContentModelBase>(ParentClass)->ParentModelClass.Get())
            {
                UE_LOG(LogPAContent, Error, TEXT("RegisterModelsByScan: %s — 계층은 2단까지. %s가 이미 자식 클래스임 — 스킵"),
                    *C->GetName(), *ParentClass->GetName());
                continue;
            }
        }
        ParentMap.Add(C, ParentClass);
    }

    // 생성 → 부모-자식 연결. 자식의 GC 참조는 부모의 Children(UPROPERTY)이 보장하므로 outer는 전부 매니저
    TMap<UClass*, UPAContentModelBase*> InstanceMap;
    for (const TPair<UClass*, UClass*>& Pair : ParentMap)
    {
        InstanceMap.Add(Pair.Key, NewObject<UPAContentModelBase>(this, Pair.Key));
    }
    for (const TPair<UClass*, UClass*>& Pair : ParentMap)
    {
        if (Pair.Value == nullptr)
        {
            RootModels.Add(InstanceMap[Pair.Key]);
        }
        else if (UPAContentModelBase** ParentInstance = InstanceMap.Find(Pair.Value))
        {
            (*ParentInstance)->AttachChild(InstanceMap[Pair.Key]);
        }
        else
        {
            // 부모가 검증에서 탈락한 자식 — 함께 스킵 (연결되지 않은 인스턴스는 GC 수거)
            UE_LOG(LogPAContent, Error, TEXT("RegisterModelsByScan: %s의 부모가 등록되지 않아 함께 스킵"), *Pair.Key->GetName());
        }
    }

    // HandleInitialize 캐스케이드 (루트 순회 — 자식 전파는 모델이 수행)
    for (UPAContentModelBase* Root : RootModels)
    {
        Root->HandleInitialize(this);
        AddToMap(Root);
    }

    UE_LOG(LogPAContent, Log, TEXT("Content models registered: %d model(s), %d root(s)"), ModelMap.Num(), RootModels.Num());
}

void UPAContentModelManagerBase::AddToMap(UPAContentModelBase* Model)
{
    ModelMap.Add(Model->GetClass(), Model);
    for (UPAContentModelBase* Child : Model->GetChildren())
    {
        AddToMap(Child);
    }
}

void UPAContentModelManagerBase::StartLogin()
{
    // 로그인 게이트(응답 도착까지 대기·실패 처리)는 두지 않았다 — 지금은 대기가 필요한 컨텐츠가 없다.
    // 첫 실사용 컨텐츠(퀘스트)가 생기는 슬라이스 2에서 실물과 함께 도입한다.
    for (UPAContentModelBase* Model : RootModels)
    {
        Model->HandleLogin(); // 자식 전파는 모델이 수행
    }

    UE_LOG(LogPAContent, Log, TEXT("Content login completed"));
    OnLoginCompleted.Broadcast();
}

void UPAContentModelManagerBase::ResetAll()
{
    // 보류 응답 폐기 먼저 — 이전 세션 응답이 리셋 이후 새 세션 데이터를 덮는 창을 닫는다
    if (Network)
    {
        Network->CancelAllRequests();
    }

    for (UPAContentModelBase* Model : RootModels)
    {
        Model->HandleReset();
    }
    // 이벤트 디스패처는 무상태 — 리셋 없음. 구독 수명은 구독자(위젯/모델)가 관리한다.
}

UGameInstance* UPAContentModelManagerBase::ResolveGameInstance(const UObject* WorldContextObject)
{
    if (!GEngine || !WorldContextObject)
    {
        return nullptr;
    }
    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    return World ? World->GetGameInstance() : nullptr;
}

UPAContentEventDispatcher* UPAContentModelManagerBase::ResolveDispatcher(
    const UObject* WorldContextObject, const TCHAR* ErrorContext)
{
    const UGameInstance* GameInstance = ResolveGameInstance(WorldContextObject);
    if (GameInstance)
    {
        // 베이스가 Abstract라 GetSubsystem<베이스>()의 정확 클래스 매칭에 걸리지 않는다
        // → 파생 배열 조회 (게임 인스턴스당 구체 매니저 1개 전제)
        for (UPAContentModelManagerBase* Manager : GameInstance->GetSubsystemArrayCopy<UPAContentModelManagerBase>())
        {
            if (Manager && Manager->GetEventDispatcher())
            {
                return Manager->GetEventDispatcher();
            }
        }
    }

    if (ErrorContext)
    {
        UE_LOG(LogPAContent, Error, TEXT("%s: 컨텐츠 매니저/디스패처를 찾을 수 없음 (WorldContext=%s)"),
            ErrorContext, WorldContextObject ? *WorldContextObject->GetName() : TEXT("null"));
    }
    return nullptr;
}

void UPAContentModelManagerBase::UnregisterContentEvent(
    const UObject* WorldContextObject, FGameplayTag Channel, FDelegateHandle Handle)
{
    if (UPAContentEventDispatcher* Dispatcher = ResolveDispatcher(WorldContextObject, nullptr))
    {
        Dispatcher->UnregisterListener(Channel, Handle);
    }
}
