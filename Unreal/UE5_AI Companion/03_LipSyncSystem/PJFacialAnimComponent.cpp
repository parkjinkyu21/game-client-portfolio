// AI Character Companion Application
// Built with Unreal Engine 5

#include "PJFacialAnimComponent.h"
#include "A2FClient/PJA2FClient.h"

DEFINE_LOG_CATEGORY(LogPJFacialAnim);

#if PLATFORM_IOS
#include "AtomAudioLinkComponent.h"
#include "LipsAtomComponent.h"
#include "AtomAudioLinkSettings.h"
#else
#include "RuntimeVisemeGenerator.h"
#endif

static constexpr float AudioSyncInterval = 0.1f;


UPJFacialAnimComponent::UPJFacialAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// RMH 15종 비짐 타겟 기본값 (FName이라 대소문자 구분 없음)
	FacialTargetNames = {
		TEXT("sil"), TEXT("pp"), TEXT("ff"), TEXT("th"), TEXT("dd"),
		TEXT("kk"),  TEXT("ch"), TEXT("ss"), TEXT("nn"), TEXT("rr"),
		TEXT("aa"),  TEXT("e"),  TEXT("ih"), TEXT("oh"), TEXT("ou")
	};
}

void UPJFacialAnimComponent::BeginPlay()
{
	Super::BeginPlay();

	StreamingSoundWave = UStreamingSoundWave::CreateStreamingSoundWave();
	StreamingSoundWave->Duration = INDEFINITELY_LOOPING_DURATION;
	StreamingSoundWave->OnAudioPlaybackFinishedNative.AddUObject(this, &ThisClass::OnPlaybackFinished);

	AudioImporterLib = URuntimeAudioImporterLibrary::CreateRuntimeAudioImporter();
	AudioImporterLib->OnResult.AddDynamic(this, &UPJFacialAnimComponent::OnAudioImportResult);

	AudioComponent = NewObject<UAudioComponent>(this, "AudioComponent");
	AudioComponent->bAutoActivate = false;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	AudioComponent->bShouldRemainActiveIfDropped = true;
	AudioComponent->Mobility = EComponentMobility::Movable;
	AudioComponent->SetupAttachment(this);

#if PLATFORM_IOS
	CRI_AtomAudioLinkComponent = NewObject<UAtomAudioLinkComponent>(this, "AtomAudioLinkComponent");
	CRI_AtomAudioLinkComponent->AtomComponentClass = ULipsAtomComponent::StaticClass();
	CRI_AtomAudioLinkComponent->bApplyExtensionSettings = true;
	CRI_AtomAudioLinkComponent->Settings = NewObject<UAtomAudioLinkSettings>(CRI_AtomAudioLinkComponent, TEXT("AtomAudioLinkSettings"));
	CRI_AtomAudioLinkComponent->SetupAttachment(this);
	// CRI 페이셜 타겟 순서를 RMH 비짐 타겟 인덱스로 매핑
	CRI_FacialTargetIndex.Add(10); // aa
	CRI_FacialTargetIndex.Add(11); // e
	CRI_FacialTargetIndex.Add(12); // ih
	CRI_FacialTargetIndex.Add(13); // oh
	CRI_FacialTargetIndex.Add(14); // ou
#else
	RMH_VisemeGenerator = URuntimeVisemeGenerator::CreateRuntimeVisemeGenerator();
#endif

	FacialTargetValues.Init(0.0f, FacialTargetNames.Num());
}

void UPJFacialAnimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UPJFacialAnimComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessVisemeInput(DeltaTime);

	ProcessAudioInput();

	ExtractFacialResults();
}


void UPJFacialAnimComponent::ProcessVisemeInput(float DeltaTime)
{
	if (PCMFloatBuffer.IsEmpty())
	{
		return;
	}

	int32 FrameSize = SampleRate * NumChannels * DeltaTime;
	int32 BufferCount = FMath::Min(PCMFloatBuffer.Num(), FrameSize);
	if (BufferCount > 0)
	{
		TArray<float> PCMData(PCMFloatBuffer.GetData(), BufferCount);
		PCMFloatBuffer.RemoveAt(0, BufferCount);

		// A2F 모드: PCM을 A2F 서비스로 전송 (CRI/RMH 비활성)
		if (FacialMode == EPJFacialMode::Audio2Face)
		{
			if (A2FClient && A2FClient->IsConnected())
			{
				A2FClient->SendAudioChunk(PCMData);
			}
			return;
		}

		// Traditional 모드: 플랫폼별 비짐 분석
#if PLATFORM_IOS

#else
		if (RMH_VisemeGenerator.Get())
		{
			RMH_VisemeGenerator->ProcessAudioData(PCMData, SampleRate, NumChannels);
		}
#endif
	}
}

// 오디오 출력을 위한 PCM 주입
void UPJFacialAnimComponent::ProcessAudioInput()
{
	if (PCMInt16Buffer.IsEmpty())
	{
		return;
	}

	int32 FrameSize = SampleRate * NumChannels * AudioSyncInterval;
	int32 BufferCount = FMath::Min(PCMInt16Buffer.Num(), FrameSize);
	if (BufferCount > 0)
	{
		TArray<int16> CurrentPCM(PCMInt16Buffer.GetData(), BufferCount);
		PCMInt16Buffer.RemoveAt(0, BufferCount);

		TArray<uint8> PCMBytes;
		PCMBytes.SetNumUninitialized(CurrentPCM.Num() * sizeof(int16));
		FMemory::Memcpy(PCMBytes.GetData(), CurrentPCM.GetData(), PCMBytes.Num());
		StreamingSoundWave->AppendAudioDataFromRAW(PCMBytes, ERuntimeRAWAudioFormat::Int16, SampleRate, NumChannels);

		if (!IsPlaying())
		{
			StartPlayback();
			if (OnFacialAnimStarted.IsBound())
			{
				OnFacialAnimStarted.Broadcast();
			}
		}
	}
}

void UPJFacialAnimComponent::ExtractFacialResults()
{
	if (FacialPlayType == EPJFacialPlayType::None)
	{
		return;
	}

#if PLATFORM_IOS
	if (CRI_AtomAudioLinkComponent.Get() && CRI_AtomAudioLinkComponent->GetAtomComponent())
	{
		if (ULipsAtomComponent* LipsAtomComp = Cast<ULipsAtomComponent>(CRI_AtomAudioLinkComponent->GetAtomComponent()))
		{
			FLipsBlendAmounts_Japanese OutBlendAmounts;
			LipsAtomComp->GetMorphTargetBlendAmountJapanese(OutBlendAmounts);
			if (CRI_FacialTargetIndex.Num() == 5 && FacialTargetValues.Num() == FacialTargetNames.Num())
			{
				FacialTargetValues[CRI_FacialTargetIndex[0]] = OutBlendAmounts.A;
				FacialTargetValues[CRI_FacialTargetIndex[1]] = OutBlendAmounts.E;
				FacialTargetValues[CRI_FacialTargetIndex[2]] = OutBlendAmounts.I;
				FacialTargetValues[CRI_FacialTargetIndex[3]] = OutBlendAmounts.O;
				FacialTargetValues[CRI_FacialTargetIndex[4]] = OutBlendAmounts.U;
			}
		}
	}

#else
	if (RMH_VisemeGenerator.Get())
	{
		TArray<float> VisemeWeights = RMH_VisemeGenerator->GetVisemeWeights();
		if (FacialTargetValues.Num() == VisemeWeights.Num())
		{
			FacialTargetValues = VisemeWeights;
		}
	}
#endif
}


bool UPJFacialAnimComponent::IsPlaying() const
{
#if PLATFORM_IOS
	if (CRI_AtomAudioLinkComponent.Get() && CRI_AtomAudioLinkComponent->IsLinkPlaying())
	{
		return true;
	}
#else
	if (AudioComponent.Get() && AudioComponent->IsPlaying())
	{
		return true;
	}
#endif

	return false;
}

void UPJFacialAnimComponent::StartPlayback()
{
	if (!StreamingSoundWave.Get())
	{
		return;
	}

#if PLATFORM_IOS
	if (CRI_AtomAudioLinkComponent.Get() && !CRI_AtomAudioLinkComponent->IsLinkPlaying())
	{
		CRI_AtomAudioLinkComponent->SetLinkSound(StreamingSoundWave);
		CRI_AtomAudioLinkComponent->PlayLink();
	}
#else
	if (AudioComponent.Get() && !AudioComponent->IsPlaying())
	{
		AudioComponent->SetSound(StreamingSoundWave);
		AudioComponent->Play();
	}
#endif
}


void UPJFacialAnimComponent::PlayFromFile(USoundBase* InSound)
{
	Stop();

	USoundWave* SoundWave = Cast<USoundWave>(InSound);
	if (SoundWave)
	{
		FString FilePath = FPaths::ProjectContentDir() + FString::Printf(TEXT("ExternalAudio/%s.wav"), *SoundWave->GetName());
		AudioImporterLib->ImportAudioFromFile(FilePath, ERuntimeAudioFormat::Wav);
	}
}

void UPJFacialAnimComponent::PlayFromImportedSoundWave(UImportedSoundWave* InSoundWave)
{
	Stop();

	FacialPlayType = EPJFacialPlayType::Wave;

	SampleRate = InSoundWave->GetSampleRate();
	NumChannels = InSoundWave->GetNumOfChannels();
	StreamingSoundWave->SetSampleRate(SampleRate);
	StreamingSoundWave->NumChannels = NumChannels;

	PCMFloatBuffer = InSoundWave->GetPCMBufferCopy();
	PCMInt16Buffer.Empty();
	int16* Int16DataPtr = nullptr;
	URuntimeAudioImporterLibrary::TranscodeRAWDataFloatToInt(PCMFloatBuffer.GetData(), PCMFloatBuffer.Num(), Int16DataPtr);
	if (Int16DataPtr)
	{
		PCMInt16Buffer = TArray<int16>(Int16DataPtr, PCMFloatBuffer.Num());
		FMemory::Free(Int16DataPtr);
	}
}

void UPJFacialAnimComponent::PlayFromBuffer(const TArray<float>& InPCMFloat, const TArray<int16>& InPCMInt16, int32 InSampleRate, int32 InNumChannels)
{
	if (FacialPlayType != EPJFacialPlayType::PCMBuffer)
	{
		Stop();

		// A2F 모드: 스트리밍 세션 시작
		if (FacialMode == EPJFacialMode::Audio2Face && A2FClient && A2FClient->IsConnected())
		{
			A2FClient->StartStream(InSampleRate, InNumChannels);
		}
	}

	FacialPlayType = EPJFacialPlayType::PCMBuffer;

	SampleRate = InSampleRate;
	NumChannels = InNumChannels;
	StreamingSoundWave->SetSampleRate(SampleRate);
	StreamingSoundWave->NumChannels = NumChannels;

	PCMFloatBuffer.Append(InPCMFloat);
	PCMInt16Buffer.Append(InPCMInt16);
}

void UPJFacialAnimComponent::Stop()
{
	// A2F 스트림 종료
	if (FacialMode == EPJFacialMode::Audio2Face && A2FClient)
	{
		A2FClient->FinishStream();
	}

#if PLATFORM_IOS
	if (CRI_AtomAudioLinkComponent.Get())
	{
		CRI_AtomAudioLinkComponent->SetLinkSound(nullptr);
		CRI_AtomAudioLinkComponent->StopLink();
	}
#else
	if (RMH_VisemeGenerator.Get())
	{
		RMH_VisemeGenerator->ClearVisemeWeights();
	}
#endif

	if (AudioComponent.Get())
	{
		AudioComponent->SetSound(nullptr);
		AudioComponent->Stop();
	}

	ExtractFacialResults();
	FacialTargetValues.Init(0.0f, FacialTargetNames.Num());

	if (FacialPlayType == EPJFacialPlayType::PCMBuffer)
	{
		if (OnFacialAnimEnded.IsBound())
		{
			OnFacialAnimEnded.Broadcast();
		}
	}

	FacialPlayType = EPJFacialPlayType::None;
	PCMFloatBuffer.Empty();
	PCMInt16Buffer.Empty();
	if (StreamingSoundWave.Get())
	{
		StreamingSoundWave->ReleaseMemory();
	}
}

void UPJFacialAnimComponent::OnAudioImportResult(URuntimeAudioImporterLibrary* Importer, UImportedSoundWave* InSoundWave, ERuntimeImportStatus Status)
{
	if (Status == ERuntimeImportStatus::SuccessfulImport)
	{
		UE_LOG(LogPJFacialAnim, Log, TEXT("Audio import succeeded!"));
		PlayFromImportedSoundWave(InSoundWave);
	}
	else
	{
		UE_LOG(LogPJFacialAnim, Warning, TEXT("Audio import failed!"));
	}
}

void UPJFacialAnimComponent::OnPlaybackFinished()
{
	if (FacialPlayType == EPJFacialPlayType::PCMBuffer)
	{
		Stop();
	}
}


// ═══════════════════════════════════════════════════════════════════════
//  Audio2Face 모드
// ═══════════════════════════════════════════════════════════════════════

void UPJFacialAnimComponent::SetFacialMode(EPJFacialMode InMode)
{
	if (FacialMode == InMode)
	{
		return;
	}

	Stop();
	FacialMode = InMode;

	if (FacialMode == EPJFacialMode::Audio2Face)
	{
		if (!A2FClient)
		{
			A2FClient = NewObject<UPJAudio2FaceClient>(this);
		}
		A2FClient->OnBlendShapeFrameReceived().AddUObject(this, &ThisClass::OnA2FBlendShapeFrameReceived);

		// A2F 모드에서는 타겟을 ARKit 52 블렌드셰이프로 교체
		const TArray<FName>& ARKitNames = UPJAudio2FaceClient::GetARKitBlendShapeNames();
		FacialTargetNames.Empty();
		for (const FName& Name : ARKitNames)
		{
			FacialTargetNames.Add(Name.ToString());
		}
		FacialTargetValues.Init(0.0f, FacialTargetNames.Num());

		UE_LOG(LogPJFacialAnim, Log, TEXT("FacialMode: Audio2Face (%d blendshapes)"), FacialTargetNames.Num());
	}
	else
	{
		// Traditional 모드: RMH 15종 비짐으로 복원
		FacialTargetNames = {
			TEXT("sil"), TEXT("pp"), TEXT("ff"), TEXT("th"), TEXT("dd"),
			TEXT("kk"),  TEXT("ch"), TEXT("ss"), TEXT("nn"), TEXT("rr"),
			TEXT("aa"),  TEXT("e"),  TEXT("ih"), TEXT("oh"), TEXT("ou")
		};
		FacialTargetValues.Init(0.0f, FacialTargetNames.Num());

		UE_LOG(LogPJFacialAnim, Log, TEXT("FacialMode: Traditional (%d visemes)"), FacialTargetNames.Num());
	}
}

void UPJFacialAnimComponent::OnA2FBlendShapeFrameReceived(const FA2FBlendShapeFrame& Frame)
{
	if (FacialMode != EPJFacialMode::Audio2Face)
	{
		return;
	}

	ApplyA2FBlendShapesToMesh(Frame);
}

void UPJFacialAnimComponent::ApplyA2FBlendShapesToMesh(const FA2FBlendShapeFrame& Frame)
{
	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// A2F 블렌드셰이프 가중치를 SkeletalMesh 모프 타겟에 직접 적용
	for (const auto& [BlendShapeName, Weight] : Frame.BlendShapeWeights)
	{
		MeshComp->SetMorphTarget(BlendShapeName, Weight);
	}

	// AnimNode에서도 읽을 수 있도록 FacialTargetValues 동기화
	const TArray<FName>& ARKitNames = UPJAudio2FaceClient::GetARKitBlendShapeNames();
	for (int32 i = 0; i < ARKitNames.Num() && i < FacialTargetValues.Num(); ++i)
	{
		if (const float* Found = Frame.BlendShapeWeights.Find(ARKitNames[i]))
		{
			FacialTargetValues[i] = *Found;
		}
	}
}
