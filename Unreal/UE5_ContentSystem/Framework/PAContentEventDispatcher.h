#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "PAContentEventDispatcher.generated.h"

// 컨텐츠 이벤트 디스패처: 도메인 상관없이 채널 기반 이벤트 전파 클래스.
// 채널 = GameplayTag, 페이로드 = USTRUCT 값 전달.
// 채널의 페이로드 타입은 첫 등록시 고정되며, 이후 다른 타입의 페이로드는 Error 로그 후 무시된다.
UCLASS()
class PACONTENTSYSTEM_API UPAContentEventDispatcher : public UObject
{
    GENERATED_BODY()

public:
    template <typename TEvent>
    FDelegateHandle RegisterListener(FGameplayTag Channel, TDelegate<void(const TEvent&)> Callback)
    {
        TDelegate<void(const void*)> Wrapper = TDelegate<void(const void*)>::CreateLambda(
            [Inner = MoveTemp(Callback)](const void* EventPtr)
            {
                Inner.ExecuteIfBound(*static_cast<const TEvent*>(EventPtr));
            });
        return AddListenerInternal(Channel, TEvent::StaticStruct(), MoveTemp(Wrapper));
    }

    void UnregisterListener(FGameplayTag Channel, FDelegateHandle Handle);

    template <typename TEvent>
    void Broadcast(FGameplayTag Channel, const TEvent& Event)
    {
        BroadcastInternal(Channel, TEvent::StaticStruct(), &Event);
    }

private:
    struct FListener
    {
        FDelegateHandle Handle;
        TDelegate<void(const void*)> Wrapper;
    };

    struct FChannelEntry
    {
        const UScriptStruct* EventType = nullptr; // 최초 등록시 정해진다.
        TArray<FListener> Listeners;
    };

    // 채널 검증 및 타입 확인. 실패 시 Error 로그 후 null을 리턴한다.
    FChannelEntry* ValidateChannel(FGameplayTag Channel, const UScriptStruct* EventType, const TCHAR* Op);

    FDelegateHandle AddListenerInternal(FGameplayTag Channel, const UScriptStruct* EventType,
        TDelegate<void(const void*)>&& Wrapper);
    void BroadcastInternal(FGameplayTag Channel, const UScriptStruct* EventType, const void* EventPtr);

    TMap<FGameplayTag, FChannelEntry> Channels;
};
