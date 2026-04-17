// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJSTTClientBase.h"
#include "PJSTTClientCloudGrpc.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PJSTTClientCloudGrpc, Log, All);

/**
 * Google Cloud Speech-to-Text gRPC 스트리밍 STT 클라이언트
 *
 * [프로토콜 흐름]
 * 1. gRPC 서비스 연결 (speech.googleapis.com:443)
 * 2. StreamingRecognize 시작: Config 전송 (인코딩, 샘플레이트, 언어 설정)
 * 3. PCM 오디오 청크를 AudioContent로 스트리밍 전송
 * 4. 서버에서 실시간 텍스트 인식 결과를 스트리밍 수신
 * 5. IsFinal 판정 시 PartialResult 콜백, 스트림 종료 시 Finished 콜백
 *
 * [특징]
 * - gRPC 기반 스트리밍 통신
 * - VoiceActivityTimeout으로 침묵 감지 후 자동 스트림 종료
 * - 에러 발생 시 자동 재연결 (ReconnectService)
 */
UCLASS()
class PJNETWORK_API UPJSTTClientCloudGrpc : public UPJSTTClientBase
{
    GENERATED_BODY()

public:
	virtual void Connect(const FString& InAuthToken) override;
	virtual void Disconnect() override;
	virtual void RequestSTT(const FPJSTTRequestParams& RequestParams) override;
	virtual bool IsConnected() override;

protected:
	UFUNCTION()
	void OnServiceStateChanged(EGrpcServiceState InServiceState);
	UFUNCTION()
	void OnStreamingFinished(const FGrpcResult& GrpcResult);
	UFUNCTION()
	void OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcGoogleCloudSpeechV1StreamingRecognizeResponse& Response);

private:
	void ReconnectService();

	UPROPERTY()
	TObjectPtr<class USpeech> STTService;
	EGrpcServiceState ServiceState = EGrpcServiceState::NotCreate;
};
