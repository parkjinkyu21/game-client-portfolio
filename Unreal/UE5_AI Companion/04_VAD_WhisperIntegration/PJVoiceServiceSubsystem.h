// AI Character Companion Application
// Built with Unreal Engine 5

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PJVoiceServiceSubsystem.generated.h"

/**
 * 음성 서비스 서브시스템 - 마이크 캡처, VAD, STT/TTS 파이프라인 관장
 *
 * [전체 음성 파이프라인]
 * Microphone -> CapturableSoundWave -> PCM 수신
 *   -> WhisperVADProcessor (발화 감지)
 *     -> 발화 시작: STT_Start (gRPC 스트리밍 시작)
 *     -> 발화 중: STT_InProgress (PCM 청크 전송)
 *     -> 발화 종료: STT_End (스트리밍 종료, 텍스트 수신)
 *       -> AI 서버로 텍스트 전송 -> TTS 응답 -> 립싱크 구동
 *
 * [주요 기능]
 * - 마이크 캡처 및 PCM 데이터 관리
 * - VAD 기반 자동 발화 구간 감지 (Whisper Silero VAD)
 * - STT 스트리밍 요청 (Start/InProgress/End 3단계)
 * - TTS 요청 및 응답 PCM -> 립싱크 컴포넌트 전달
 * - 녹음된 음성 재생 (최대 3개 버퍼 유지)
 */

class APJVoiceActor;
class UAudioComponent;
class UStreamingSoundWave;
class UCapturableSoundWave;
class USoundWaveProcedural;
class UPJWhisperVADProcessor;
class USoundBase;

enum class EPJGenderType : uint8;
enum class EPJLanguageEnum : uint8;
enum class EPJTTSClientType : uint8;


DECLARE_LOG_CATEGORY_EXTERN(PJVoiceServiceSubsystem, Log, All);


UENUM()
enum class EPJVoiceEventType :uint8
{
	Started,		// 시작됨
	Finished,		// 정상 종료
	PartialResult,	// 스트리밍 처리시 부분 결과를 받을 때
	FinalResult,	// 끝나면 모든 결과를 다시 보내줌.
	Error,			// 오류 발생
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVoiceTTSEvent, int64, RequestID, EPJVoiceEventType, EventType, const TArray<uint8>&, AudioBuffer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVoiceSTTEvent, int64, RequestID, EPJVoiceEventType, EventType, const FString&, Result);

UCLASS()
class PJCORE_API UPJVoiceServiceSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	FOnVoiceTTSEvent OnVoiceTTSEvent;
	FOnVoiceSTTEvent OnVoiceSTTEvent;

	//override function from UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//override from FTickableGameObject
	virtual bool IsTickable() const override { return bIsInitialized; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return GetStatID(); }

	void RegisterVoiceActor(APJVoiceActor* InActor);
	void UnregisterVoiceActor();

	// STT 요청 (3단계 스트리밍: Start -> InProgress -> End)
	int64 RequestSTT_Start(int32 SampleRate, int32 ChannelCount);
	void RequestSTT_InProgress(const TArray<uint8>& PCMBuffer);
	void RequestSTT_End();
	// TTS 요청: message를 한번에 보내기 때문에 Start/InProgress/End를 내부에서 자동 처리
	int64 RequestTTS(const FString& Message);

	bool StartMicrophone();
	void StopMicrophone();

	void PlayCapturedVoice(int32 VoiceIndex);
	void StopCapturedVoice();
	void SetCaptureMute(bool bMute);

	void PlayPCMAudio(const TArray<uint8>& PCMData);
	void PlayAudio(USoundBase* Sound);

	int32	GetCurrentVoiceModelId() { return CurrentVoiceModelId; }
	void	SetCurrentVoiceModelId(int32 ID) { CurrentVoiceModelId = ID; }
	void	SetCurrentVoiceModelId(EPJGenderType Gender, EPJLanguageEnum LanguageType);
	void	SetTTSClientType(EPJTTSClientType Type) { TTSClientType = Type;}
	EPJTTSClientType	GetTTSClientType() { return TTSClientType; }
	bool	IsEnableTTS();

	TArray<uint8> GetPCMData(int32 RequestID);

protected:
	UFUNCTION()
	void OnMicrophoneDataReceived(const TArray<float>& Data);
	UFUNCTION()
	void OnMicrophoneSpeechStarted();
	UFUNCTION()
	void OnMicrophoneSpeechEnded();
	UFUNCTION()
	void OnVoiceTTSEventInternal(int64 RequestID, EPJVoiceEventType EventType, const TArray<uint8>& AudioBuffer);
	UFUNCTION()
	void OnVoiceSTTEventInternal(int64 RequestID, EPJVoiceEventType EventType, const FString& Result);

private:
	// VAD 발화 감지 후 PCM을 일정 간격(0.2초)으로 모아서 STT 스트리밍 전송
	void CapturedVoiceRequestSTT(bool bStop = false);
	void ClearMicReleaseMemoryTimerHandle();

private:

	bool bIsInitialized = false;

	TWeakObjectPtr<APJVoiceActor>	VoiceActor;

	UPROPERTY()
	TObjectPtr<UCapturableSoundWave>	MicrophoneCapture;
	int32 MicrophoneSampleRate = 0;
	int32 MicrophoneChannel = 0;
	FTimerHandle MicReleaseMemoryTimerHandle;

	UPROPERTY()
	TObjectPtr<UAudioComponent> PlaybackAudioComponent;
	UPROPERTY(Transient)
	TObjectPtr<UStreamingSoundWave>	MicPcmBufferPlaybackSoundWave;
	TArray<TArray<uint8>> MicPcmPlaybackBuffers;

	TArray<uint8> TTSPcmBuffer;
	TMap<int32, TArray<uint8>> PcmDataMap;
	TObjectPtr<USoundWaveProcedural> ProceduralSoundWave;

	TArray<float> CurrentPCMBuffer;
	// 마이크로 받은 오디오 버퍼를 STT 처리한 Index
	int32 CurrentPCMBufferIndex = 0;
	bool bIsMicrophoneSpeaking = false;
	int64 VoiceRequestID = 1;

	int32 CurrentVoiceModelId = 0;

	TArray<uint8> STTRequestPcmBuffer;

	const int32 TTSSampleRate = 24000;

	EPJTTSClientType TTSClientType;

	UPROPERTY()
	TObjectPtr<UPJWhisperVADProcessor>	WhisperVADProcessor;
};