// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJSpeechClientDefine.h"
#include "PJSTTClientBase.generated.h"

/**
 * STT 클라이언트 베이스 클래스 (Strategy 패턴)
 *
 * - Connect/Disconnect/RequestSTT: 각 프로바이더가 오버라이드하는 가상 인터페이스
 * - TTS 클라이언트(UPJTTSClientBase)와 동일한 설계 패턴 적용
 * - 서브클래스: CloudGrpc (Google Cloud Speech-to-Text gRPC 스트리밍)
 */
UCLASS()
class PJNETWORK_API UPJSTTClientBase : public UObject
{
    GENERATED_BODY()

public:
	virtual void Connect(const FString& InAuthToken) { AuthToken = InAuthToken; }
	virtual void Disconnect() {}
	virtual void RequestSTT(const FPJSTTRequestParams& RequestParams) {}
	virtual bool IsConnected() { return false; }

protected:
	FString AuthToken;
	int64 STTRequestID = 0;
	FPJSTTResponseLambda STTResponseLambda;
	FString STTRecognizeText;
};
