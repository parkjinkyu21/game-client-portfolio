// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJTTSClientBase.h"
#include "PJTTSClientCloudGrpc.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PJTTSClientCloudGrpc, Log, All);


UCLASS(Blueprintable)
class PJNETWORK_API UPJTTSClientCloudGrpc : public UPJTTSClientBase
{
    GENERATED_BODY()

public:
	virtual void Connect(const FString& InAuthToken) override;

    virtual void RequestTTS(const FPJTTSRequestParams& RequestParams) override;

protected:
	// TTS 서비스 상태 변경
	UFUNCTION()
	void OnServiceStateChanged(EGrpcServiceState InServiceState);
	// TTS 스트리밍 종료
	UFUNCTION()
	void OnStreamingFinished(const FGrpcResult& GrpcResult);
	// TTS 스트리밍 리스폰
	UFUNCTION()
	void OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcGoogleCloudTexttospeechV1StreamingSynthesizeResponse& Response);

private:

	UPROPERTY()
	TObjectPtr<class UTextToSpeech>	TTSService;	
	EGrpcServiceState ServiceState;

};