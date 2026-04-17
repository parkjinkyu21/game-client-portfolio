// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJA2FClient.h"

DEFINE_LOG_CATEGORY(LogPJA2FClient);

const TArray<FName>& UPJAudio2FaceClient::GetARKitBlendShapeNames()
{
	static TArray<FName> Names = {
		// 눈
		TEXT("eyeBlinkLeft"),      TEXT("eyeBlinkRight"),
		TEXT("eyeLookDownLeft"),    TEXT("eyeLookDownRight"),
		TEXT("eyeLookInLeft"),      TEXT("eyeLookInRight"),
		TEXT("eyeLookOutLeft"),     TEXT("eyeLookOutRight"),
		TEXT("eyeLookUpLeft"),      TEXT("eyeLookUpRight"),
		TEXT("eyeSquintLeft"),      TEXT("eyeSquintRight"),
		TEXT("eyeWideLeft"),        TEXT("eyeWideRight"),
		// 입
		TEXT("jawForward"),         TEXT("jawLeft"),
		TEXT("jawRight"),           TEXT("jawOpen"),
		TEXT("mouthClose"),         TEXT("mouthFunnel"),
		TEXT("mouthPucker"),        TEXT("mouthLeft"),
		TEXT("mouthRight"),         TEXT("mouthSmileLeft"),
		TEXT("mouthSmileRight"),    TEXT("mouthFrownLeft"),
		TEXT("mouthFrownRight"),    TEXT("mouthDimpleLeft"),
		TEXT("mouthDimpleRight"),   TEXT("mouthStretchLeft"),
		TEXT("mouthStretchRight"),  TEXT("mouthRollLower"),
		TEXT("mouthRollUpper"),     TEXT("mouthShrugLower"),
		TEXT("mouthShrugUpper"),    TEXT("mouthPressLeft"),
		TEXT("mouthPressRight"),    TEXT("mouthLowerDownLeft"),
		TEXT("mouthLowerDownRight"),TEXT("mouthUpperUpLeft"),
		TEXT("mouthUpperUpRight"),
		// 눈썹
		TEXT("browDownLeft"),       TEXT("browDownRight"),
		TEXT("browInnerUp"),        TEXT("browOuterUpLeft"),
		TEXT("browOuterUpRight"),
		// 볼/코/혀
		TEXT("cheekPuff"),          TEXT("cheekSquintLeft"),
		TEXT("cheekSquintRight"),   TEXT("noseSneerLeft"),
		TEXT("noseSneerRight"),     TEXT("tongueOut"),
	};

	return Names;
}

void UPJAudio2FaceClient::Connect(const FString& EndPoint)
{
	ReconnectService();
}

void UPJAudio2FaceClient::Disconnect()
{
	A2FService = nullptr;
	ServiceState = EGrpcServiceState::NotCreate;
}

bool UPJAudio2FaceClient::IsConnected() const
{
	return (ServiceState == EGrpcServiceState::Ready);
}

void UPJAudio2FaceClient::ReconnectService()
{
	UGrpcServiceManager* GrpcManager = UGrpcUtilities::GetGrpcServiceManager(UPJNetworkManager::Get());
	if (A2FService.Get())
	{
		GrpcManager->ReleaseService(A2FService, true);
		A2FService = nullptr;
	}

	GrpcManager->SetServiceEndPoint(TEXT("Audio2Face"), EndPoint);
	A2FService = Cast<UAudio2Face>(GrpcManager->MakeService("Audio2Face"));
	A2FService->Connect();

	UAudio2FaceClient* A2FClient = A2FService->MakeClient();
	A2FClient->OnProcessAudioStreamFinished.AddDynamic(this, &ThisClass::OnStreamingFinished);
	A2FClient->OnProcessAudioStreamResponse.AddDynamic(this, &ThisClass::OnStreamingResponse);
	A2FService->OnServiceStateChanged.AddDynamic(this, &ThisClass::OnServiceStateChanged);

	OnServiceStateChanged(A2FService->GetServiceState());
}

void UPJAudio2FaceClient::OnServiceStateChanged(EGrpcServiceState InServiceState)
{
	if (ServiceState != InServiceState)
	{
		ServiceState = InServiceState;
		UE_LOG(LogPJA2FClient, Log, TEXT("A2F ServiceState: %d"), (int32)ServiceState);
	}
}


// ═══════════════════════════════════════════════════════════════════════
//  스트리밍 세션
// ═══════════════════════════════════════════════════════════════════════

void UPJAudio2FaceClient::StartStream(int32 SampleRate, int32 NumChannels)
{
	if (!A2FService || !IsConnected())
	{
		return;
	}

	CurrentSampleRate = SampleRate;
	CurrentNumChannels = NumChannels;

	A2FService->InitProcessAudioStream(FString());

	// Config 전송: 오디오 포맷 설정
	FGrpcNvidiaA2FProcessAudioStreamRequest ConfigRequest;
	ConfigRequest.RequestType.Config.SampleRate = SampleRate;
	ConfigRequest.RequestType.Config.NumChannels = NumChannels;
	ConfigRequest.RequestType.Config.AudioFormat = EGrpcNvidiaA2FAudioFormat::PCM_FLOAT;
	ConfigRequest.RequestType.RequestTypeCase = EGrpcNvidiaA2FProcessAudioStreamRequestType::Config;

	A2FService->CallProcessAudioStream(ConfigRequest, nullptr);

	UE_LOG(LogPJA2FClient, Log, TEXT("StartStream (SampleRate: %d, Channels: %d)"), SampleRate, NumChannels);
}

void UPJAudio2FaceClient::SendAudioChunk(const TArray<float>& PCMBuffer)
{
	if (!A2FService || PCMBuffer.IsEmpty())
	{
		return;
	}

	FGrpcNvidiaA2FProcessAudioStreamRequest AudioRequest;
	AudioRequest.RequestType.AudioData.PCMData.Value.SetNumUninitialized(PCMBuffer.Num() * sizeof(float));
	FMemory::Memcpy(AudioRequest.RequestType.AudioData.PCMData.Value.GetData(), PCMBuffer.GetData(), AudioRequest.RequestType.AudioData.PCMData.Value.Num());
	AudioRequest.RequestType.RequestTypeCase = EGrpcNvidiaA2FProcessAudioStreamRequestType::AudioData;

	A2FService->CallProcessAudioStream(AudioRequest, nullptr);
}

void UPJAudio2FaceClient::FinishStream()
{
	if (!A2FService)
	{
		return;
	}

	// 빈 AudioData를 보내 스트림 종료를 알림
	FGrpcNvidiaA2FProcessAudioStreamRequest EndRequest;
	EndRequest.RequestType.RequestTypeCase = EGrpcNvidiaA2FProcessAudioStreamRequestType::AudioData;

	A2FService->CallProcessAudioStream(EndRequest, nullptr);

	UE_LOG(LogPJA2FClient, Log, TEXT("FinishStream"));
}


// ═══════════════════════════════════════════════════════════════════════
//  스트리밍 응답 처리
// ═══════════════════════════════════════════════════════════════════════

void UPJAudio2FaceClient::OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcNvidiaA2FProcessAudioStreamResponse& Response)
{
	if (GrpcResult.Code != EGrpcResultCode::Ok)
	{
		UE_LOG(LogPJA2FClient, Error, TEXT("A2F Response Error: [%s] %s"), *GrpcResult.GetCodeString(), *GrpcResult.GetMessageString());
		return;
	}

	// A2F 서버 응답에서 블렌드셰이프 가중치 추출
	FA2FBlendShapeFrame Frame;
	Frame.TimeStamp = Response.TimeStamp;

	const TArray<FName>& BlendShapeNames = GetARKitBlendShapeNames();
	const TArray<float>& Weights = Response.BlendShapeWeights;

	int32 Count = FMath::Min(BlendShapeNames.Num(), Weights.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		Frame.BlendShapeWeights.Add(BlendShapeNames[i], Weights[i]);
	}

	BlendShapeFrameDelegate.Broadcast(Frame);
}

void UPJAudio2FaceClient::OnStreamingFinished(const FGrpcResult& GrpcResult)
{
	if (GrpcResult.Code == EGrpcResultCode::Ok || GrpcResult.Code == EGrpcResultCode::Cancelled)
	{
		UE_LOG(LogPJA2FClient, Log, TEXT("A2F Stream Finished"));
	}
	else
	{
		UE_LOG(LogPJA2FClient, Error, TEXT("A2F Stream Error: [%s] %s"), *GrpcResult.GetCodeString(), *GrpcResult.Message);
		ReconnectService();
	}

	StreamFinishedDelegate.Broadcast();
}
