// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJWhisperVADProcessor.h"

// Whisper 서드파티 라이브러리 헤더
// Whisper를 빌드하여 ThirdParty/Whisper/에 배치
#include "whisper.h"
#include "SampleBuffer.h"

DEFINE_LOG_CATEGORY(PJWhisperVADProcessor);


// 커스텀 모델 로더: Whisper 라이브러리에 메모리 버퍼에서 직접 모델을 로드하기 위한 콜백 구현
struct FModelMemoryStream
{
	const uint8* Data;
	size_t Size;
	size_t Position;

	FModelMemoryStream(const uint8* InData, size_t InSize)
		: Data(InData), Size(InSize), Position(0) {
	}
};

// Read 콜백: 모델 바이너리에서 요청된 크기만큼 읽기
size_t ModelLoader_Read(void* Ctx, void* Output, size_t ReadSize)
{
	FModelMemoryStream* Stream = (FModelMemoryStream*)Ctx;

	size_t BytesLeft = Stream->Size - Stream->Position;
	if (BytesLeft < ReadSize)
	{
		UE_LOG(PJWhisperVADProcessor, Error, TEXT("ModelLoader_Read: insufficient data. Requested %zu, available %zu"), ReadSize, BytesLeft);
		return 0;
	}

	FMemory::Memcpy(Output, Stream->Data + Stream->Position, ReadSize);
	Stream->Position += ReadSize;
	return ReadSize;
}

// EOF 콜백: 모델 데이터 끝 도달 여부 확인
bool ModelLoader_EOF(void* Ctx)
{
	FModelMemoryStream* Stream = (FModelMemoryStream*)Ctx;
	return Stream->Position >= Stream->Size;
}

// Close 콜백: 메모리 관리는 외부에서 하므로 별도 처리 없음
void ModelLoader_Close(void* Ctx)
{
}



bool UPJWhisperVADProcessor::Initialize()
{
	// Silero VAD 모델(GGML 포맷)을 메모리에 로드
	FString VADModelPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Whisper/ggml-silero-v5.1.2.bin"));
	TArray<uint8> VADModelData;
	if (!FFileHelper::LoadFileToArray(VADModelData, *VADModelPath))
	{
		UE_LOG(PJWhisperVADProcessor, Error, TEXT("Whisper model not found: %s"), *VADModelPath);
		return false;
	}

	// 커스텀 모델 로더를 통해 메모리에서 직접 VAD 컨텍스트 생성
	FModelMemoryStream Stream = FModelMemoryStream(VADModelData.GetData(), VADModelData.Num());
	whisper_model_loader Loader = {};
	Loader.context = (void*)(&Stream);
	Loader.read = &ModelLoader_Read;
	Loader.eof = &ModelLoader_EOF;
	Loader.close = &ModelLoader_Close;

	whisper_vad_context_params VADParams = whisper_vad_default_context_params();
	WhisperVADCtx = whisper_vad_init_with_params(&Loader, VADParams);

	bIsSpeaking = false;

	// 프로젝트 설정에서 VAD 파라미터 로드
	if (const UPJGameConstant* GameConstant = GetDefault<UPJGameConstant>())
	{
		VADCycleTime = GameConstant->VADSetting.VADCycleTime;
		SpeechStartThreshold = GameConstant->VADSetting.SpeechStartThreshold;
		SpeechEndThreshold = GameConstant->VADSetting.SpeechEndThreshold;
		SilenceTimeoutSec = GameConstant->VADSetting.SilenceTimeoutSec;
	}

	UE_LOG(PJWhisperVADProcessor, Log, TEXT("Whisper SpeechProcessor initialized successfully"));
	return true;
}

void UPJWhisperVADProcessor::UpdateVAD(float DeltaTime)
{
	if (WhisperVADCtx == nullptr)
	{
		return;
	}

	ElapsedTimeForVAD += DeltaTime;

	// VADCycleTime(0.1초) 주기 도달 시 실제 VAD 처리 실행
	if (ElapsedTimeForVAD >= VADCycleTime)
	{
		ProcessVAD(ElapsedTimeForVAD);

		ElapsedTimeForVAD = 0.f;
	}
}

// 핵심 VAD 처리 로직
// 이전 프레임 + 현재 프레임의 PCM을 합쳐서 처리하여 경계 구간의 검출 정확도를 향상
void UPJWhisperVADProcessor::ProcessVAD(float DeltaTime)
{
	if (CurPCMBuffer.IsEmpty())
	{
		return;
	}

	bool bNowSpeaking = false;
	bool bNowSilence = true;

	// 이전 프레임 버퍼 + 현재 프레임 버퍼를 합쳐서 검출 정확도 향상
	TArray<float> PCMBuffer = PrePCMBuffer;
	PCMBuffer.Append(CurPCMBuffer.GetData(), CurPCMBuffer.Num());

	// Whisper VAD API 호출: PCM 데이터에서 음성 활동 확률값 추출
	if (whisper_vad_detect_speech(WhisperVADCtx, PCMBuffer.GetData(), PCMBuffer.Num()))
	{
		int32 NumProbs = whisper_vad_n_probs(WhisperVADCtx);
		float* Probs = whisper_vad_probs(WhisperVADCtx);
		if (NumProbs > 0)
		{
			for (int32 i = 0; i < NumProbs; i++)
			{
				float Prob = Probs[i];
				// 확률값이 발화 임계값 이상이면 발화로 판정
				if (Prob >= SpeechStartThreshold)
				{
					bNowSpeaking = true;
				}
				// 확률값이 침묵 임계값 이상이면 아직 침묵이 아님
				if (Prob >= SpeechEndThreshold)
				{
					bNowSilence = false;
				}
			}
		}
	}
	else
	{
		UE_LOG(PJWhisperVADProcessor, Warning, TEXT("whisper_vad_detect_speech Failed"));
	}

	// 침묵 시간 누적
	if (bNowSilence)
		SilenceElapsedTime += DeltaTime;
	else
		SilenceElapsedTime = 0.f;

	// 발화 시작 이벤트
	if (bNowSpeaking && !bIsSpeaking)
	{
		bIsSpeaking = true;
		OnVADSpeakingStart.Broadcast();
		UE_LOG(PJWhisperVADProcessor, Log, TEXT("Speech Start"));
	}

	// 발화 종료 이벤트 (침묵이 SilenceTimeoutSec 이상 지속되면)
	if (bNowSilence && bIsSpeaking)
	{
		if (SilenceElapsedTime >= SilenceTimeoutSec)
		{
			bIsSpeaking = false;
			OnVADSpeakingEnd.Broadcast();
			UE_LOG(PJWhisperVADProcessor, Log, TEXT("Speech End"));
		}
	}

	// 현재 버퍼를 이전 버퍼로 이동 (다음 사이클에서 합산 처리용)
	PrePCMBuffer = CurPCMBuffer;
	CurPCMBuffer.Empty();

}

// 마이크 PCM 데이터를 VAD 분석용 포맷(16kHz 모노)으로 변환하여 버퍼에 추가
void UPJWhisperVADProcessor::AddPCM(const TArray<float>& PCMData, int32 SampleRate, int32 Channel)
{
	if (WhisperVADCtx == nullptr)
	{
		return;
	}

	// Whisper Silero VAD 모델은 16kHz 모노로 학습되었으므로 리샘플링 필요
	TArray<float> ConvertedPCMData;
	ConvertPCMData(PCMData, ConvertedPCMData, SampleRate, Channel, 16000, 1);

	CurPCMBuffer.Append(ConvertedPCMData.GetData(), ConvertedPCMData.Num());
}
void UPJWhisperVADProcessor::StartVAD()
{

}

void UPJWhisperVADProcessor::StopVAD()
{
	if (bIsSpeaking)
	{
		OnVADSpeakingEnd.Broadcast();
	}

	ResetVAD();
}

void UPJWhisperVADProcessor::ResetVAD()
{
	bIsSpeaking = false;
	CurPCMBuffer.Empty();
	PrePCMBuffer.Empty();
	SilenceElapsedTime = 0.f;
	ElapsedTimeForVAD = 0.f;
}


void UPJWhisperVADProcessor::BeginDestroy()
{
	if (WhisperVADCtx)
	{
		whisper_vad_free(WhisperVADCtx);
		WhisperVADCtx = nullptr;
	}

	Super::BeginDestroy();
}

// PCM 리샘플링: Audio::TSampleBuffer를 활용한 채널 믹싱 + 샘플레이트 변환
void UPJWhisperVADProcessor::ConvertPCMData(const TArray<float>& SrcPCMData, TArray<float>& OutConvertedData, int32 SourceSampleRate, int32 SourceChannel, int32 TargetSampleRate, int32 TargetNumChannels)
{
	if (SrcPCMData.Num() == 0)
	{
		return;
	}

	// UE Audio::TSampleBuffer에 원본 PCM 로드
	Audio::TSampleBuffer<float> SampleBuffer(SrcPCMData.GetData(), SrcPCMData.Num(), SourceChannel, SourceSampleRate);

	// 채널 믹싱 (예: 2ch -> 1ch)
	if (SourceChannel != TargetNumChannels)
	{
		SampleBuffer.MixBufferToChannels(TargetNumChannels);
	}

	// Audio::Resample API를 사용한 리샘플링
	Audio::FAlignedFloatBuffer RemixedRAWData = Audio::FAlignedFloatBuffer(SampleBuffer.GetData(), SampleBuffer.GetNumSamples());
	const Audio::FResamplingParameters ResampleParameters = {
		Audio::EResamplingMethod::BestSinc,
		TargetNumChannels,
		static_cast<float>(SampleBuffer.GetSampleRate()),
		static_cast<float>(TargetSampleRate),
		RemixedRAWData
	};

	Audio::FAlignedFloatBuffer AlignedConvertedData;
	AlignedConvertedData.AddUninitialized(Audio::GetOutputBufferSize(ResampleParameters));

	Audio::FResamplerResults ResampleResults;
	ResampleResults.OutBuffer = &AlignedConvertedData;

	if (!Audio::Resample(ResampleParameters, ResampleResults))
	{
		UE_LOG(PJWhisperVADProcessor, Error, TEXT("Unable to resample audio data from %dHz to %dHz"), SampleBuffer.GetSampleRate(), TargetSampleRate);
	}

	OutConvertedData.Empty();
	if (SourceChannel != TargetNumChannels)
	{
		// 채널 믹싱 후 볼륨 보상 (2배 증폭, 클리핑 방지)
		for (float Sample : AlignedConvertedData)
		{
			float Temp = FMath::Clamp(Sample * 2, -1.0f, 1.0f);
			OutConvertedData.Add(Temp);
		}
	}
	else
	{
		OutConvertedData.Append(AlignedConvertedData.GetData(), AlignedConvertedData.Num());
	}
}
