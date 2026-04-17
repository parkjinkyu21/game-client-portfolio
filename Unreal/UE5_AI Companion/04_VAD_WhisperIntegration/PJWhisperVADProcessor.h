// AI Character Companion Application
// Built with Unreal Engine 5

#pragma once

#include "CoreMinimal.h"
#include "PJWhisperVADProcessor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PJWhisperVADProcessor, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPJOnVADSpeakingStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPJOnVADSpeakingEnd);

/**
 * Whisper Silero VAD 프로세서
 *
 * Whisper 라이브러리의 Silero VAD 모델(ggml-silero-v5.1.2.bin)을 활용한
 * 실시간 음성 활동 감지(Voice Activity Detection) 프로세서.
 *
 * [Whisper 서드파티 통합]
 * - whisper.cpp 오픈소스를 빌드하여 ThirdParty 라이브러리로 통합
 * - whisper.lib, ggml-base.lib, ggml-cpu.lib, ggml.lib 링크
 * - Win64/Android 플랫폼별 빌드 설정 (Build.cs에서 관리)
 * - 커스텀 모델 로더 콜백(Read/EOF/Close)으로 메모리에서 직접 로드
 *
 * [VAD 알고리즘]
 * 1. 마이크 PCM 데이터를 16kHz/모노로 리샘플링 (Audio::Resample 사용)
 * 2. VADCycleTime(0.1초) 주기로 PCM 버퍼를 누적
 * 3. 이전 프레임 + 현재 프레임을 합쳐서 whisper_vad_detect_speech() 호출
 * 4. 확률값 기반 발화/침묵 판정:
 *    - prob >= SpeechStartThreshold(0.35) -> 발화 시작
 *    - prob < SpeechEndThreshold(0.2) + SilenceTimeout(0.75s) -> 발화 종료
 * 5. OnVADSpeakingStart/End 델리게이트로 이벤트 브로드캐스트
 */
UCLASS()
class PJCORE_API UPJWhisperVADProcessor : public UObject
{
    GENERATED_BODY()

public:
    bool Initialize();
	// 마이크에서 수신한 PCM을 VAD 분석 버퍼에 추가 (내부에서 16kHz 모노로 리샘플링)
	void AddPCM(const TArray<float>& PCMData, int32 SampleRate, int32 Channel);
	void StartVAD();
	void StopVAD();

	// DeltaTime 누적해서 VADCycleTime 주기 도달 시 VAD 처리
	void UpdateVAD(float DeltaTime);

	FPJOnVADSpeakingStart OnVADSpeakingStart;
    FPJOnVADSpeakingEnd OnVADSpeakingEnd;

protected:
    virtual void BeginDestroy() override;

private:
	// 핵심 VAD 처리: 이전+현재 PCM 버퍼를 합쳐서 Whisper VAD에 전달하고 확률값 판정
	void ProcessVAD(float DeltaTime);
	void ResetVAD();

	// PCM 리샘플링: 임의의 샘플레이트/채널 -> 16kHz/모노 변환 (Audio::TSampleBuffer 활용)
	void ConvertPCMData(const TArray<float>& SrcPCMData, TArray<float>& OutConvertedData, int32 SourceSampleRate, int32 SourceChannel, int32 TargetSampleRate, int32 TargetNumChannels);
private:
	// Whisper VAD 컨텍스트 (Silero VAD 모델 인스턴스)
	struct whisper_vad_context* WhisperVADCtx = nullptr;

    bool bIsSpeaking = false;

	// VAD 처리를 위한 PCM 수집 주기 (초)
	float VADCycleTime = 0.1f;
	// VADCycleTime까지 누적된 시간
	float ElapsedTimeForVAD = 0.f;
	// 발화로 판단하는 확률 임계값
	float SpeechStartThreshold = 0.35f;
	// 침묵으로 판단하는 확률 임계값
	float SpeechEndThreshold = 0.2f;
	// 발화 이후 침묵이 지속되면 발화 종료로 판단하는 시간
	float SilenceTimeoutSec = 0.75f;
	// 현재 침묵 유지 시간
	float SilenceElapsedTime = 0.f;

	// 이전+현재 버퍼를 합쳐 처리하여 검출 정확도 향상
	TArray<float> CurPCMBuffer;
	TArray<float> PrePCMBuffer;
};
