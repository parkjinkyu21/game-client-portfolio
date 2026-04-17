// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJTTSClientBase.h"
#include "PJTTSClientMinimaxWebSocket.generated.h"

class IWebSocket;


DECLARE_LOG_CATEGORY_EXTERN(PJTTSClientMinimaxWebSocket, Log, All);

/**
 * Minimax WebSocket 기반 TTS 클라이언트
 *
 * [프로토콜 흐름]
 * TaskStart(음성 설정) -> TaskContinue(텍스트 전송) -> 오디오 수신 -> TaskFinish
 * - 연결 제한시간 120초이므로 TTS 요청 1건당 Connection 1회 생성
 * - Hex 인코딩된 PCM 오디오 데이터를 바이너리로 디코딩
 * - 프리버퍼링 후 PartialResult로 점진적 오디오 전달
 */
UCLASS(Blueprintable)
class PJNETWORK_API UPJTTSClientMinimaxWebSocket : public UPJTTSClientBase
{
    GENERATED_BODY()

public:
	virtual void Connect(const FString& InAuthToken) override;
	virtual void Disconnect() override;
    virtual void RequestTTS(const FPJTTSRequestParams& RequestParams) override;
private:
	void Connect();
	void OnConnected();
    void OnMessage(const FString& Message);
	void OnRawMessage(const void* Data, SIZE_T Size, SIZE_T BytesRemaining);
	void OnBinaryMessage(const void* Data, SIZE_T Size, bool bIsLastFragment);
    void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void OnConnectionError(const FString& Reason);
	void OnTTSResponse(EPJSpeechResponseState ResponseState);
	void ResetTTSInfo();
	FString GetAudioDataFromServerContent(const TSharedPtr<FJsonObject>* ServerContent, bool& bGenerationComplete);


	void SendTaskStart();
	void SendContinue();
	void SendFinish();
	void ReceiveContinued(const TSharedPtr<FJsonObject>& JsonObject);

protected:
	UPROPERTY(Transient)
	FString APIKey;
	UPROPERTY(Transient)
	FString GroupID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	FString LLMModelName = TEXT("speech-2.5-hd-preview");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	float VoiceSpeed = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	float VoiceVolum = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	int32 VoicePitch = 0;
	// 스트리밍 재생 시작 전에 미리 확보해 둘 오디오 버퍼 시간(초 단위)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	float PreBufferDurationSec = 0.1f;
private:
	TSharedPtr<IWebSocket>	WebSocket;
	bool					bCloseRequested = false;

	TArray<uint8> TTSSynthesizePartialBuffer;

	// connect 이후 요청을 위하여.
	FPJTTSRequestParams BackupRequestParams;
};