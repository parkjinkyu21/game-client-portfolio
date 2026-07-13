#include "PAContentEventDispatcher.h"
#include "PAContentSystem.h"

UPAContentEventDispatcher::FChannelEntry* UPAContentEventDispatcher::ValidateChannel(
    FGameplayTag Channel, const UScriptStruct* EventType, const TCHAR* Op)
{
    if (!Channel.IsValid())
    {
        UE_LOG(LogPAContent, Error, TEXT("%s: 무효 채널 태그 — 무시"), Op);
        return nullptr;
    }

    FChannelEntry& Entry = Channels.FindOrAdd(Channel);
    if (Entry.EventType == nullptr)
    {
        Entry.EventType = EventType; // 최초 사용 — 채널 타입 고정
    }
    else if (Entry.EventType != EventType)
    {
        UE_LOG(LogPAContent, Error, TEXT("%s: 채널 %s 타입 불일치 — 고정 %s, 시도 %s — 무시"),
            Op, *Channel.ToString(), *Entry.EventType->GetName(),
            EventType ? *EventType->GetName() : TEXT("null"));
        return nullptr;
    }
    return &Entry;
}

FDelegateHandle UPAContentEventDispatcher::AddListenerInternal(
    FGameplayTag Channel, const UScriptStruct* EventType, TDelegate<void(const void*)>&& Wrapper)
{
    FChannelEntry* Entry = ValidateChannel(Channel, EventType, TEXT("RegisterListener"));
    if (!Entry)
    {
        return FDelegateHandle(); // 무효 핸들
    }

    const FDelegateHandle Handle = Wrapper.GetHandle();
    Entry->Listeners.Add(FListener{ Handle, MoveTemp(Wrapper) });
    return Handle;
}

void UPAContentEventDispatcher::UnregisterListener(FGameplayTag Channel, FDelegateHandle Handle)
{
    if (!Handle.IsValid())
    {
        return;
    }
    if (FChannelEntry* Entry = Channels.Find(Channel))
    {
        Entry->Listeners.RemoveAll([&Handle](const FListener& Listener)
        {
            return Listener.Handle == Handle;
        });
        // 채널 엔트리는 남긴다 — 타입 고정을 유지해 이후 오등록도 계속 잡는다
    }
}

void UPAContentEventDispatcher::BroadcastInternal(
    FGameplayTag Channel, const UScriptStruct* EventType, const void* EventPtr)
{
    FChannelEntry* Entry = ValidateChannel(Channel, EventType, TEXT("Broadcast"));
    if (!Entry || Entry->Listeners.IsEmpty())
    {
        return;
    }

    // 콜백이 등록/해지를 재진입해도 안전하도록 스냅샷을 하여 순회한다.
    // 도중 해지된 리스너는 건너뜀
    TArray<FListener> Snapshot = Entry->Listeners;
    for (const FListener& Listener : Snapshot)
    {
        const FChannelEntry* Live = Channels.Find(Channel);
        const bool bStillRegistered = Live && Live->Listeners.ContainsByPredicate(
            [&Listener](const FListener& L) { return L.Handle == Listener.Handle; });
        if (bStillRegistered)
        {
            Listener.Wrapper.ExecuteIfBound(EventPtr);
        }
    }
}
