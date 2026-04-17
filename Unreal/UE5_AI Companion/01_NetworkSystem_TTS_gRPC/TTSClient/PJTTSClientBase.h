// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJTTSClientBase.generated.h"

/**
 * TTS 클라이언트 베이스 클래스 (Strategy + Factory 패턴)
 *
 * - CreateClient(): Factory Method로 EPJTTSClientType에 따라 구체 클라이언트 생성
 * - Connect/Disconnect/RequestTTS: 각 프로바이더가 오버라이드하는 가상 인터페이스
 * - 서브클래스: GeminiWebSocket, MinimaxWebSocket, CloudGrpc, GeminiLiveHTTP
 */
UCLASS(Blueprintable)
class PJNETWORK_API UPJTTSClientBase : public UObject
{
    GENERATED_BODY()

public:
	virtual ~UPJTTSClientBase();

	// Factory Method: TTS 프로바이더 타입에 따라 적절한 클라이언트 인스턴스 생성
	static UPJTTSClientBase* CreateClient(EPJTTSClientType ClientType);

	virtual void Connect(const FString& InAuthToken) { SetAuthToken(InAuthToken); }
	virtual void Disconnect(){}
    virtual void RequestTTS(const FPJTTSRequestParams& RequestParams);
	void SetAuthToken(const FString& InAuthToken) { AuthToken = InAuthToken; }

	void SetClientType(EPJTTSClientType InClientType) { ClientType = InClientType;}
	EPJTTSClientType GetClientType() { return ClientType; }


protected:
	FString	AuthToken;
	FString	VoiceLanguage;
	FString	VoiceName;
	int32	SampleRate = 24000;
	int64 TTSRequestID = 0;
	FPJTTSResponseLambda TTSResponseLambda;
	TArray<uint8> TTSSynthesizeFinalAudioBuffer;

	EPJTTSClientType ClientType = EPJTTSClientType::None;
};