// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJVoiceServiceSubsystem.h"

DEFINE_LOG_CATEGORY(PJVoiceServiceSubsystem);

const int32 MicPlaybackPcmBufferMax = 3;

void UPJVoiceServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	if (bIsInitialized) return;

	TTSClientType = EPJTTSClientType::MinimaxWebSocket;
	VoiceRequestID = 1;

	// 마이크 캡처 초기화
	MicrophoneCapture = UCapturableSoundWave::CreateCapturableSoundWave();
	MicrophoneCapture->OnPopulateAudioData.AddDynamic(this, &ThisClass::OnMicrophoneDataReceived);

	MicPcmBufferPlaybackSoundWave = UStreamingSoundWave::CreateStreamingSoundWave();
	MicPcmBufferPlaybackSoundWave->Duration = INDEFINITELY_LOOPING_DURATION;

	OnVoiceTTSEvent.AddDynamic(this, &ThisClass::OnVoiceTTSEventInternal);
	OnVoiceSTTEvent.AddDynamic(this, &ThisClass::OnVoiceSTTEventInternal);

	CurrentVoiceModelId = 1;

	// Whisper VAD 프로세서 초기화 및 발화 이벤트 바인딩
	WhisperVADProcessor = NewObject<UPJWhisperVADProcessor>();
	WhisperVADProcessor->Initialize();
	WhisperVADProcessor->OnVADSpeakingStart.AddDynamic(this, &ThisClass::OnMicrophoneSpeechStarted);
	WhisperVADProcessor->OnVADSpeakingEnd.AddDynamic(this, &ThisClass::OnMicrophoneSpeechEnded);

	bIsInitialized = true;

	UE_LOG(PJVoiceServiceSubsystem, Log, TEXT("Initialized "));
}

void UPJVoiceServiceSubsystem::Deinitialize()
{
	if (WhisperVADProcessor.Get())
	{
		WhisperVADProcessor = nullptr;
	}

	if (MicrophoneCapture.Get())
	{
		MicrophoneCapture = nullptr;
	}

	ClearMicReleaseMemoryTimerHandle();

	UE_LOG(PJVoiceServiceSubsystem, Log, TEXT("Deinitialized "));
	bIsInitialized = false;
}

void UPJVoiceServiceSubsystem::Tick(float DeltaTime)
{
	if (!bIsInitialized) return;

	// 매 프레임 VAD 업데이트 (내부에서 VADCycleTime 주기로 실제 처리)
	if (WhisperVADProcessor.Get())
	{
		WhisperVADProcessor->UpdateVAD(DeltaTime);
	}
}

void UPJVoiceServiceSubsystem::RegisterVoiceActor(APJVoiceActor* InActor)
{
	UnregisterVoiceActor();

	if (InActor && InActor->GetRootComponent())
	{
		PlaybackAudioComponent = NewObject<UAudioComponent>(GetTransientPackage());
		PlaybackAudioComponent->bAutoActivate = false;
		PlaybackAudioComponent->RegisterComponentWithWorld(GetWorld());

		USceneComponent* Root = InActor->GetRootComponent();
		if (Root)
		{
			PlaybackAudioComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			PlaybackAudioComponent->SetRelativeLocation(FVector::ZeroVector);
		}
	}

	VoiceActor = InActor;
}

void UPJVoiceServiceSubsystem::UnregisterVoiceActor()
{
	if (PlaybackAudioComponent.Get())
	{
		PlaybackAudioComponent->Stop();
		PlaybackAudioComponent->DestroyComponent();
		PlaybackAudioComponent = nullptr;
	}

	VoiceActor.Reset();
}

bool UPJVoiceServiceSubsystem::StartMicrophone()
{
	bool bResult = false;

	if (MicrophoneCapture.Get() && !MicrophoneCapture->IsCapturing())
	{
		bResult = MicrophoneCapture->StartCapture(0);
		MicrophoneSampleRate = MicrophoneCapture->GetSampleRate();
		MicrophoneChannel = MicrophoneCapture->GetNumOfChannels();

		bIsMicrophoneSpeaking = false;
		CurrentPCMBufferIndex = 0;
		CurrentPCMBuffer.Empty();

		if (WhisperVADProcessor.Get())
		{
			WhisperVADProcessor->StartVAD();
		}

		ClearMicReleaseMemoryTimerHandle();

		// 버퍼가 계속 쌓이므로 60초 주기로 메모리 해제
		FTimerDelegate MicReleaseMemoryDelegate = FTimerDelegate::CreateLambda([WeakThis = MakeWeakObjectPtr(this)]()
			{
				if (WeakThis.IsValid())
				{
					MicrophoneCapture->ResetAudio();
					MicrophoneCapture->ReleaseMemory();
				}
			});
		GetWorld()->GetTimerManager().SetTimer(MicReleaseMemoryTimerHandle, MicReleaseMemoryDelegate, 60.f, true);
	}

	return bResult;
}

void UPJVoiceServiceSubsystem::ClearMicReleaseMemoryTimerHandle()
{
	if (MicReleaseMemoryTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(MicReleaseMemoryTimerHandle);
	}
}

void UPJVoiceServiceSubsystem::StopMicrophone()
{
	if (MicrophoneCapture.Get())
	{
		if (WhisperVADProcessor.Get())
		{
			WhisperVADProcessor->StopVAD();
		}
		MicrophoneCapture->StopCapture();
		MicrophoneCapture->ReleaseMemory();
		MicrophoneCapture->ResetAudio();

	}

	ClearMicReleaseMemoryTimerHandle();
}

void UPJVoiceServiceSubsystem::OnMicrophoneDataReceived(const TArray<float>& Data)
{
	if (!MicrophoneCapture.Get() || !MicrophoneCapture->IsCapturing())
	{
		return;
	}

	MicrophoneSampleRate = MicrophoneCapture->GetSampleRate();
	MicrophoneChannel = MicrophoneCapture->GetNumOfChannels();

	// 마이크 PCM 데이터를 VAD 프로세서에 전달
	if (WhisperVADProcessor.Get())
	{
		WhisperVADProcessor->AddPCM(Data, MicrophoneSampleRate, MicrophoneChannel);
	}

	CurrentPCMBuffer.Append(Data.GetData(), Data.Num());

	// VAD가 발화를 감지하지 않은 상태에서는 버퍼만 유지
	if (!bIsMicrophoneSpeaking)
	{
		// STT 요청시 첫 글자가 짤리지 않도록 발화 시작 전 0.3초 분량의 버퍼를 유지
		static float BufferSec = 0.3f;
		int32 BufferCount = MicrophoneSampleRate * MicrophoneChannel * BufferSec;
		if(CurrentPCMBuffer.Num() > BufferCount)
		{
			CurrentPCMBuffer.RemoveAt(0, CurrentPCMBuffer.Num() - BufferCount);
		}
		return;
	}

	CapturedVoiceRequestSTT();
}

void UPJVoiceServiceSubsystem::OnMicrophoneSpeechStarted()
{
	CurrentPCMBufferIndex = 0;
	bIsMicrophoneSpeaking = true;
}

void UPJVoiceServiceSubsystem::OnMicrophoneSpeechEnded()
{
	if (bIsMicrophoneSpeaking)
	{
		CapturedVoiceRequestSTT(true);
		bIsMicrophoneSpeaking = false;
	}
}

// VAD 발화 감지 후 PCM을 0.2초 간격으로 모아서 STT 스트리밍 전송
void UPJVoiceServiceSubsystem::CapturedVoiceRequestSTT(bool bStop)
{
	static float RequestIntervalSec = 0.2f;
	int32 RequestBufferCount = MicrophoneSampleRate * MicrophoneChannel * RequestIntervalSec;
	int32 BufferCount = CurrentPCMBuffer.Num() - CurrentPCMBufferIndex;
	if (!bStop)
	{
		if( BufferCount < RequestBufferCount)
			return;

		BufferCount = RequestBufferCount;
	}

	// STT 시작
	if (CurrentPCMBufferIndex == 0)
	{
		RequestSTT_Start(MicrophoneSampleRate, MicrophoneChannel);
	}

	// STT 스트리밍 (float -> int16 변환 후 전송)
	if (BufferCount > 0)
	{
		TArray<uint8> PCMBytes;
		int16* Int16DataPtr = nullptr;
		URuntimeAudioImporterLibrary::TranscodeRAWDataFloatToInt(CurrentPCMBuffer.GetData() + CurrentPCMBufferIndex, BufferCount, Int16DataPtr);
		if (Int16DataPtr)
		{
			PCMBytes = TArray<uint8>((uint8*)Int16DataPtr, BufferCount * sizeof(int16));
			FMemory::Free(Int16DataPtr);
		}
		RequestSTT_InProgress(PCMBytes);
	}

	// STT 종료
	if (bStop)
	{
		RequestSTT_End();
	}

	CurrentPCMBufferIndex += BufferCount;

}

void UPJVoiceServiceSubsystem::PlayCapturedVoice(int32 VoiceIndex)
{
	StopCapturedVoice();

	if (VoiceIndex < 0 || VoiceIndex >= MicPcmPlaybackBuffers.Num())
	{
		return;
	}

	if (MicPcmBufferPlaybackSoundWave.Get() && MicrophoneCapture.Get())
	{
		MicPcmBufferPlaybackSoundWave->ReleaseMemory();
		MicPcmBufferPlaybackSoundWave->AppendAudioDataFromRAW(MicPcmPlaybackBuffers[VoiceIndex], ERuntimeRAWAudioFormat::Int16, MicrophoneCapture->GetSampleRate(), MicrophoneCapture->GetNumOfChannels());
		if (PlaybackAudioComponent.Get())
		{
			PlaybackAudioComponent->SetSound(MicPcmBufferPlaybackSoundWave);
			PlaybackAudioComponent->SetVolumeMultiplier(2.f);
			PlaybackAudioComponent->Play();
		}
	}
}

void UPJVoiceServiceSubsystem::StopCapturedVoice()
{
	if (PlaybackAudioComponent.Get())
	{
		PlaybackAudioComponent->SetSound(nullptr);
		PlaybackAudioComponent->Stop();
	}
}

void UPJVoiceServiceSubsystem::SetCaptureMute(bool bMute)
{
	if (MicrophoneCapture.Get())
	{
		MicrophoneCapture->ToggleMute(bMute);
	}
}

void UPJVoiceServiceSubsystem::PlayPCMAudio(const TArray<uint8>& PCMData)
{
	if (PCMData.IsEmpty())
	{
		return;
	}

	if (!IsValid(ProceduralSoundWave))
	{
		ProceduralSoundWave = NewObject<USoundWaveProcedural>();
	}

	ProceduralSoundWave->SetSampleRate(TTSSampleRate);
	ProceduralSoundWave->NumChannels = 1;
	ProceduralSoundWave->Duration = INDEFINITELY_LOOPING_DURATION;
	ProceduralSoundWave->SoundGroup = SOUNDGROUP_Default;
	ProceduralSoundWave->bLooping = false;

	ProceduralSoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

	if (PlaybackAudioComponent.Get())
	{
		PlaybackAudioComponent->SetSound(ProceduralSoundWave);
		PlaybackAudioComponent->SetVolumeMultiplier(2.f);
		PlaybackAudioComponent->Play();
	}
}

void UPJVoiceServiceSubsystem::PlayAudio(USoundBase* Sound)
{
	if (PlaybackAudioComponent.Get())
	{
		if (PlaybackAudioComponent->IsPlaying())
		{
			PlaybackAudioComponent->Stop();
		}

		PlaybackAudioComponent->SetSound(Sound);
		PlaybackAudioComponent->Play();
	}
}

TArray<uint8> UPJVoiceServiceSubsystem::GetPCMData(int32 RequestID)
{
	if (const TArray<uint8>* Buffer = PcmDataMap.Find(RequestID))
	{
		return *Buffer;
	}

	return {};
}

int64 UPJVoiceServiceSubsystem::RequestSTT_Start(int32 SampleRate, int32 ChannelCount)
{
	FPJSTTResponseLambda ResponseLambda = [WeakThis = MakeWeakObjectPtr(this)](int64 InRequestID, EPJSpeechResponseState ResponseState, const FString& InResult)
		{
			UE_LOG(PJVoiceServiceSubsystem, Log, TEXT("STT Response : RequestID[%d] "), InRequestID);
			UE_LOG(PJVoiceServiceSubsystem, Log, TEXT("STT Response : Text:[%s] "), *InResult);

			if (WeakThis.IsValid())
			{
				if (ResponseState == EPJSpeechResponseState::PartialResult)
				{
					WeakThis->OnVoiceSTTEvent.Broadcast(InRequestID, EPJVoiceEventType::PartialResult, InResult);
				}
				else if (ResponseState == EPJSpeechResponseState::Finished)
				{
					WeakThis->OnVoiceSTTEvent.Broadcast(InRequestID, EPJVoiceEventType::FinalResult, InResult);
					WeakThis->OnVoiceSTTEvent.Broadcast(InRequestID, EPJVoiceEventType::Finished, TEXT(""));
				}
				else if (ResponseState == EPJSpeechResponseState::Error)
				{
					WeakThis->OnVoiceSTTEvent.Broadcast(InRequestID, EPJVoiceEventType::Error, InResult);
				}
			}
		};

	if (UPJNetworkManager::Get())
	{
		FPJSTTRequestParams Params;
		Params.RequestID = VoiceRequestID;
		Params.ResponseLambda = ResponseLambda;
		Params.SampleRate = SampleRate;
		Params.ChannelCount = ChannelCount;
		Params.State = EPJSpeechRequestState::Start;

		UPJNetworkManager::Get()->RequestSTT(Params);
	}

	STTRequestPcmBuffer.Empty();

	OnVoiceSTTEvent.Broadcast(VoiceRequestID, EPJVoiceEventType::Started, TEXT(""));

	return VoiceRequestID++;
}

void UPJVoiceServiceSubsystem::RequestSTT_InProgress(const TArray<uint8>& PCMBuffer)
{
	if (UPJNetworkManager::Get())
	{
		FPJSTTRequestParams Params;
		Params.RequestID = VoiceRequestID;
		Params.PcmBuffer = PCMBuffer;
		Params.State = EPJSpeechRequestState::InProgress;

		UPJNetworkManager::Get()->RequestSTT(Params);
	}

	STTRequestPcmBuffer.Append(PCMBuffer.GetData(), PCMBuffer.Num());
}

void UPJVoiceServiceSubsystem::RequestSTT_End()
{
	if (UPJNetworkManager::Get())
	{
		FPJSTTRequestParams Params;
		Params.RequestID = VoiceRequestID;
		Params.State = EPJSpeechRequestState::End;

		UPJNetworkManager::Get()->RequestSTT(Params);
	}
}


int64 UPJVoiceServiceSubsystem::RequestTTS(const FString& Message)
{
	if (TTSClientType == EPJTTSClientType::None)
	{
		return 0;
	}

	FPJTTSResponseLambda ResponseLambda = [WeakThis = MakeWeakObjectPtr(this)](int64 InRequestID, EPJSpeechResponseState ResponseState, const TArray<uint8>& InResult)
		{
			if (WeakThis.IsValid())
			{
				if (ResponseState == EPJSpeechResponseState::PartialResult)
				{
					WeakThis->OnVoiceTTSEvent.Broadcast(InRequestID, EPJVoiceEventType::PartialResult, InResult);
				}
				else if (ResponseState == EPJSpeechResponseState::Finished)
				{
					WeakThis->OnVoiceTTSEvent.Broadcast(InRequestID, EPJVoiceEventType::FinalResult, InResult);
					WeakThis->OnVoiceTTSEvent.Broadcast(InRequestID, EPJVoiceEventType::Finished, InResult);

				}
				else if (ResponseState == EPJSpeechResponseState::Error)
				{
					WeakThis->OnVoiceTTSEvent.Broadcast(InRequestID, EPJVoiceEventType::Error, InResult);
				}
			}
		};

	if (UPJNetworkManager::Get())
	{
		FPJTTSRequestParams Params;
		if (VoiceActor.IsValid())
		{
			Params.Gender = VoiceActor->GetGenderType();
		}

		Params.VoiceModelId = CurrentVoiceModelId;
		Params.RequestID = VoiceRequestID;
		Params.Text = Message;
		Params.SampleRate = TTSSampleRate;
		Params.ResponseLambda = ResponseLambda;
		Params.TTSClientType = TTSClientType;

		// TTS는 메시지를 한번에 전송하므로 Start/InProgress/End를 순차 호출
		Params.State = EPJSpeechRequestState::Start;
		UPJNetworkManager::Get()->RequestTTS(Params);
		Params.State = EPJSpeechRequestState::InProgress;
		UPJNetworkManager::Get()->RequestTTS(Params);
		Params.State = EPJSpeechRequestState::End;
		UPJNetworkManager::Get()->RequestTTS(Params);
	}

	OnVoiceTTSEvent.Broadcast(VoiceRequestID, EPJVoiceEventType::Started, TArray<uint8>());

	return VoiceRequestID++;
}

// TTS 응답 수신 시 PCM을 립싱크 컴포넌트에 전달
void UPJVoiceServiceSubsystem::OnVoiceTTSEventInternal(int64 RequestID, EPJVoiceEventType EventType, const TArray<uint8>& AudioBuffer)
{
	TArray<uint8> PCMData;

	if (EventType == EPJVoiceEventType::Started)
	{
		// TTS 시작시 기존 립싱크 종료
		if (VoiceActor.IsValid() && VoiceActor->GetFacialAnimComponent())
		{
			UPJFacialAnimComponent* FacialAnimComp = VoiceActor->GetFacialAnimComponent();
			FacialAnimComp->Stop();
		}
	}
	else if (EventType == EPJVoiceEventType::PartialResult)
	{
		PCMData = AudioBuffer;

		TTSPcmBuffer.Append(AudioBuffer.GetData(), AudioBuffer.Num());
	}
	else if (EventType == EPJVoiceEventType::FinalResult)
	{
		PcmDataMap.Add(RequestID, TTSPcmBuffer);
		TTSPcmBuffer.Empty();

		// 마지막에 0.5초 분량의 무음을 추가하여 립싱크 잔여 데이터 방지
		PCMData.SetNumZeroed(TTSSampleRate);
	}

	if (PCMData.IsEmpty())
	{
		return;
	}

	// PCM int16 -> float 변환 후 립싱크 컴포넌트에 전달
	ensure(PCMData.Num() % 2 == 0);
	int32 NumSamples = PCMData.Num() / 2;

	TArray<int16> PCMInt16;
	TArray<float> PCMFloat;
	PCMInt16.SetNumUninitialized(NumSamples);
	PCMFloat.SetNumUninitialized(NumSamples);

	const int16* Int16Ptr = reinterpret_cast<const int16*>(PCMData.GetData());

	for (int32 i = 0; i < NumSamples; ++i)
	{
		int16 Sample = Int16Ptr[i];
		PCMInt16[i] = Sample;
		PCMFloat[i] = Sample / 32768.0f;
	}

	if (VoiceActor.IsValid() && VoiceActor->GetFacialAnimComponent())
	{
		UPJFacialAnimComponent* FacialAnimComp = VoiceActor->GetFacialAnimComponent();
		FacialAnimComp->PlayFromBuffer(PCMFloat, PCMInt16, TTSSampleRate,1);
	}
}


void UPJVoiceServiceSubsystem::OnVoiceSTTEventInternal(int64 RequestID, EPJVoiceEventType EventType, const FString& Result)
{
	if (EventType == EPJVoiceEventType::Error || EventType == EPJVoiceEventType::Finished)
	{
		MicPcmPlaybackBuffers.Add(STTRequestPcmBuffer);
		STTRequestPcmBuffer.Empty();

		if (MicPlaybackPcmBufferMax < MicPcmPlaybackBuffers.Num())
		{
			MicPcmPlaybackBuffers.RemoveAt(0);
		}
	}
}

bool UPJVoiceServiceSubsystem::IsEnableTTS()
{
	return TTSClientType != EPJTTSClientType::None;
}

void UPJVoiceServiceSubsystem::SetCurrentVoiceModelId(EPJGenderType Gender, EPJLanguageEnum LanguageType)
{
	TArray<FPJVoiceModelData*> VoiceDatas = UPJDataTables::ToValueArray<UPJVoiceModelDataTable>();
	for (const FPJVoiceModelData* VoiceData : VoiceDatas)
	{
		if (VoiceData->Gender == Gender && VoiceData->LangEnum == LanguageType)
		{
			CurrentVoiceModelId = VoiceData->Key;
		}
	}
}