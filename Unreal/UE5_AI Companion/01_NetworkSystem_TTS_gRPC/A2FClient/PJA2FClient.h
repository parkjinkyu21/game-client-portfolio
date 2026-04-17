// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJA2FClient.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPJA2FClient, Log, All);

/**
 * NVIDIA Audio2Face gRPC 스트리밍 클라이언트
 *
 * [개요]
 * TTS에서 생성된 PCM 오디오를 A2F 서비스로 스트리밍 전송하고,
 * ARKit 호환 52 블렌드셰이프 가중치를 실시간으로 수신하여
 * 립싱크와 페이셜 익스프레션을 동시에 구동합니다.
 *
 * [프로토콜 흐름]
 * 1. gRPC 서비스 연결 (A2F 서버 엔드포인트)
 * 2. ProcessAudioStream 양방향 스트리밍 시작
 * 3. PCM 오디오 청크를 스트리밍 전송
 * 4. 서버에서 프레임 단위 블렌드셰이프 가중치 수신
 * 5. 스트림 종료 시 Finished 콜백
 *
 * [A2F 출력 - ARKit 52 블렌드셰이프]
 * - 눈: eyeBlinkLeft/Right, eyeWideLeft/Right, eyeSquintLeft/Right ...
 * - 입: jawOpen, mouthSmileLeft/Right, mouthFunnel, mouthPucker ...
 * - 눈썹: browDownLeft/Right, browInnerUp, browOuterUpLeft/Right
 * - 볼/코: cheekPuff, cheekSquintLeft/Right, noseSneerLeft/Right
 * → 기존 CRI/RMH 비짐 기반 대비 훨씬 풍부한 페이셜 표현 가능
 */

// A2F 블렌드셰이프 프레임 데이터
struct FA2FBlendShapeFrame
{
	TMap<FName, float> BlendShapeWeights;
	float TimeStamp = 0.f;
};

UCLASS()
class PJNETWORK_API UPJAudio2FaceClient : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBlendShapeFrameReceived, const FA2FBlendShapeFrame&);
	DECLARE_MULTICAST_DELEGATE(FOnStreamFinished);

	void Connect(const FString& EndPoint);
	void Disconnect();
	bool IsConnected() const;

	// 스트리밍 세션 시작 (샘플레이트, 채널 설정)
	void StartStream(int32 SampleRate, int32 NumChannels);
	// PCM 오디오 청크 전송
	void SendAudioChunk(const TArray<float>& PCMBuffer);
	// 스트리밍 세션 종료
	void FinishStream();

	FOnBlendShapeFrameReceived& OnBlendShapeFrameReceived() { return BlendShapeFrameDelegate; }
	FOnStreamFinished& OnStreamFinished() { return StreamFinishedDelegate; }

	// ARKit 52 블렌드셰이프 타겟 이름 목록
	static const TArray<FName>& GetARKitBlendShapeNames();

protected:
	UFUNCTION()
	void OnServiceStateChanged(EGrpcServiceState ServiceState);
	UFUNCTION()
	void OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcNvidiaA2FProcessAudioStreamResponse& Response);
	UFUNCTION()
	void OnStreamingFinished(const FGrpcResult& GrpcResult);

private:
	void ReconnectService();

	UPROPERTY()
	TObjectPtr<class UAudio2Face> A2FService;

	EGrpcServiceState ServiceState = EGrpcServiceState::NotCreate;
	int32 CurrentSampleRate = 0;
	int32 CurrentNumChannels = 0;

	FOnBlendShapeFrameReceived BlendShapeFrameDelegate;
	FOnStreamFinished StreamFinishedDelegate;
};
