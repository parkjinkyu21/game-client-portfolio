// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJTTSClientBase.h"
#include "PJTTSClientGeminiWebSocket.generated.h"

class IWebSocket;


DECLARE_LOG_CATEGORY_EXTERN(PJTTSClientGeminiWebSocket, Log, All);

/**
 * Google Gemini WebSocket 기반 실시간 스트리밍 TTS 클라이언트
 *
 * [프로토콜 흐름]
 * 1. WebSocket 연결 -> Setup JSON 전송 (모델, 음성 설정)
 * 2. ClientContent JSON으로 텍스트 전송
 * 3. 서버에서 바이너리 오디오 청크를 스트리밍 수신
 * 4. PreBufferDurationSec만큼 오디오를 선버퍼링 후 재생 시작
 * 5. generationComplete 플래그로 스트리밍 종료 감지
 *
 * [특징]
 * - JSON 기반 핸드셰이크 및 메시지 프로토콜
 * - 프리버퍼링으로 끊김 없는 오디오 재생
 * - PCM 바이너리 청크 수신 및 누적 처리
 */
UCLASS()
class PJNETWORK_API UPJTTSClientGeminiWebSocket : public UPJTTSClientBase
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

	TSharedPtr<FJsonObject> MakeClientContentJsonObject(const FString& Message);
	TSharedPtr<FJsonObject> MakeSetupJsonObject();
	TSharedPtr<FJsonObject> MakeRolePartsJsonObject(const FString& Role, const FString& Text);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules", meta=(ToolTip="v1alpha or v1beta"))
	FString APIVersion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	FString LLMModelName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules", meta=(MultiLine=true))
	FString SystemInstruction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules", meta=(MultiLine=true))
	FString MessageInstruction;
	// 스트리밍 재생 시작 전에 미리 확보해 둘 오디오 버퍼 시간(초 단위)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TTS Rules")
	float PreBufferDurationSec = 0.2f;

private:
	TSharedPtr<IWebSocket>	WebSocket;
	bool					bCloseRequested = false;

	FString CurrentVoiceName;

	int32 TTSSampleRate = 24000;

	TArray<uint8> TTSSynthesizePartialBuffer;

	FString CurrentJsonMessage;

	// connect 이후 요청을 위하여.
	FPJTTSRequestParams BackupRequestParams;
};