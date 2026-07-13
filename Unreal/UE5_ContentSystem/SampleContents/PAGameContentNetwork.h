#pragma once

#include "CoreMinimal.h"
#include "Framework/PAContentNetworkBase.h"
#include "PAGameContentNetwork.generated.h"

// 게임 컨텐츠의 실서버 통신 클래스.
// 서버 연동 시 ReceivePacket(서버 패킷 수신)과 SendRequestBytes(요청 전송)에 프로토콜 처리를 구현한다.
UCLASS()
class PACONTENTSYSTEM_API UPAGameContentNetwork : public UPAContentNetworkBase
{
    GENERATED_BODY()

public:
    UPAGameContentNetwork();

    // 서버 패킷 수신 진입점 — 게임 네트워크 계층이 호출한다.
    // 패킷 헤더의 프로토콜 id를 메시지 id로 변환하여 ReceiveMessage를 호출한다.
    void ReceivePacket(const TArray<uint8>& PacketBytes);

protected:
    // 요청 바이트를 서버 패킷으로 만들어 전송한다.
    virtual void SendRequestBytes(FName MessageId, TArray<uint8>&& RequestBytes) override;

private:
    // 프로토콜 id와 컨텐츠 메시지 id의 변환 테이블
    TMap<uint16, FName> ProtocolTable;
};
