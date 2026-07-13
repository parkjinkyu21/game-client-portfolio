#include "PAContentNetworkBase.h"
#include "PAContentSystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UPAContentNetworkBase::PostInitProperties()
{
    Super::PostInitProperties();

    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        WorldTickStartHandle = FWorldDelegates::OnWorldTickStart.AddWeakLambda(this,
            [this](UWorld* World, ELevelTick, float)
            {
                // 자기 게임 인스턴스의 월드 틱만 (에디터 월드 등 무시)
                const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
                if (GameInstance && GameInstance == GetTypedOuter<UGameInstance>())
                {
                    ProcessReceiveBuffer();
                }
            });
    }
}

void UPAContentNetworkBase::BeginDestroy()
{
    if (WorldTickStartHandle.IsValid())
    {
        FWorldDelegates::OnWorldTickStart.Remove(WorldTickStartHandle);
        WorldTickStartHandle.Reset();
    }
    Super::BeginDestroy();
}

void UPAContentNetworkBase::ReceiveMessage(
    EPAContentRequestResult Result, FName MessageId, TArray<uint8>&& MessageBytes)
{
    FReceivedPacket& Packet = ReceiveBuffer.AddDefaulted_GetRef();
    Packet.Result = Result;
    Packet.MessageId = MessageId;
    Packet.Bytes = MoveTemp(MessageBytes);
}

void UPAContentNetworkBase::ProcessReceiveBuffer()
{
    if (ReceiveBuffer.IsEmpty())
    {
        return;
    }

    // 버퍼를 통째로 가져와 처리 — 처리 중 도착분은 새 버퍼에 쌓여 다음 처리로
    TArray<FReceivedPacket> Draining = MoveTemp(ReceiveBuffer);
    ReceiveBuffer.Reset();

    for (FReceivedPacket& Packet : Draining)
    {
        if (const FPAContentMessageDelegate* Found = MessageHandlers.Find(Packet.MessageId))
        {
            // MessageHandlers의 등록은 초기화(BindPacketHandlers) 시점뿐이고 패킷을 받았다는 의미는 초기화 시점이 지났다는 의미이기 때문에 참조로 사용해도 문제 없다.
            Found->ExecuteIfBound(Packet.Result, Packet.Bytes);
        }
        else
        {
            UE_LOG(LogPAContent, Error, TEXT("PAContentNetworkBase: no message handler for %s"), *Packet.MessageId.ToString());
        }
    }
}

void UPAContentNetworkBase::CancelAllRequests()
{
    ReceiveBuffer.Reset(); // 콜백 없이 폐기
}

void UPAContentNetworkBase::RegisterMessageHandlerBytes(FName MessageId, FPAContentMessageDelegate Handler)
{
    check(!MessageId.IsNone());

    if (MessageHandlers.Contains(MessageId))
    {
        // 패킷의 소유 도메인은 하나 — 중복은 등록 실수. 먼저 등록한 쪽을 유지한다.
        UE_LOG(LogPAContent, Error, TEXT("PAContentNetworkBase: duplicate message handler for %s - ignored"), *MessageId.ToString());
        return;
    }

    MessageHandlers.Add(MessageId, MoveTemp(Handler));
}
