#include "PAGameContentNetwork.h"
#include "SampleContents/1_Mail_SingleModel/MailContentMessages.h"
#include "SampleContents/2_Collection_MultiModel/CollectionContentMessages.h"
#include "PAContentSystem.h"
#include "Serialization/MemoryReader.h"

UPAGameContentNetwork::UPAGameContentNetwork()
{
    // 프로토콜 id 와 매칭되는 MessageId 를 등록한다.
    // 이 테이블과 메시지 정의({도메인}ContentMessages.h)는 실서버 프로토콜 스키마에서 코드 생성으로 대체가 필요하다.
    ProtocolTable.Add(1001, FMailSummaryRequest().GetMessageId());
    ProtocolTable.Add(1002, FMailListRequest().GetMessageId());
    ProtocolTable.Add(1003, FMailReadRequest().GetMessageId());
    ProtocolTable.Add(1101, FMailNewMailNotify().GetMessageId());

    ProtocolTable.Add(2001, FCollectionSummaryRequest().GetMessageId());
    ProtocolTable.Add(2002, FAppearanceListRequest().GetMessageId());
    ProtocolTable.Add(2003, FAppearanceRegisterRequest().GetMessageId());
    ProtocolTable.Add(2004, FTitleListRequest().GetMessageId());
    ProtocolTable.Add(2005, FTitleEquipRequest().GetMessageId());
    ProtocolTable.Add(2101, FTitleAcquiredNotify().GetMessageId());
    ProtocolTable.Add(2102, FAppearanceAcquiredNotify().GetMessageId());
}

// 아래 코드는 패킷 버퍼를 메시지로 변환 하는 과정을 보여주기 위한 임시 코드
void UPAGameContentNetwork::ReceivePacket(const TArray<uint8>& PacketBytes)
{
    constexpr int32 HeaderSize = sizeof(uint16) + sizeof(uint8);
    if (PacketBytes.Num() < HeaderSize)
    {
        UE_LOG(LogPAContent, Error, TEXT("PAGameContentNetwork: packet too small (%d bytes)"), PacketBytes.Num());
        return;
    }

    FMemoryReader Reader(PacketBytes);
    uint16 ProtocolId = 0;
    uint8 ResultCode = 0;
    Reader << ProtocolId;
    Reader << ResultCode;

    const FName* MessageId = ProtocolTable.Find(ProtocolId);
    if (!MessageId)
    {
        UE_LOG(LogPAContent, Error, TEXT("PAGameContentNetwork: unknown protocol id %u"), ProtocolId);
        return;
    }

    // 헤더 뒤 나머지가 메시지 바이트
    TArray<uint8> MessageBytes(PacketBytes.GetData() + HeaderSize, PacketBytes.Num() - HeaderSize);

    const EPAContentRequestResult Result =
        (ResultCode == 0) ? EPAContentRequestResult::Success : EPAContentRequestResult::Failed;
    ReceiveMessage(Result, *MessageId, MoveTemp(MessageBytes));
}

void UPAGameContentNetwork::SendRequestBytes(FName MessageId, TArray<uint8>&& RequestBytes)
{
    // 실서버 프로토콜에 맞춰 구현 — 메시지 id를 프로토콜 id로 변환해 패킷을 만들어 전송한다.
}
