// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJSpeechClientDefine.h"
#include "PJSpeechClient.generated.h"


struct FGrpcPJAgentServerAUTHKEY;
class UPJTTSClientBase;
class UPJSTTClientBase;

/**
 * TTS/STT 통합 클라이언트 (Aggregator)
 *
 * - 멀티 프로바이더 TTS: Auth Key에 따라 적절한 TTS 클라이언트로 라우팅
 * - STT: STT 클라이언트로 위임 (현재 Google Cloud Speech gRPC)
 * - 서버에서 Auth Key가 변경되면 자동으로 클라이언트 재연결
 */

DECLARE_LOG_CATEGORY_EXTERN(PJSpeechClient, Log, All);


UCLASS()
class PJNETWORK_API UPJSpeechClient : public UObject
{
	GENERATED_BODY()

public:
	void Init();
	void Clear();

	void RequestTTS(const FPJTTSRequestParams& RequestParams);
	void RequestSTT(const FPJSTTRequestParams& RequestParams);

	void OnServiceAuthKeyChanged(const TArray<FGrpcPJAgentServerAUTHKEY>& AuthKeys);

private:
	FString GetTTSClientAuthToken(EPJTTSClientType ClientType);

private:
	FString  CloudAuthKey;
	FString  GeminiAuthKey;

	UPROPERTY()
	TObjectPtr<UPJTTSClientBase> CurrentTTSClient;

	UPROPERTY()
	TObjectPtr<UPJSTTClientBase> CurrentSTTClient;
};
