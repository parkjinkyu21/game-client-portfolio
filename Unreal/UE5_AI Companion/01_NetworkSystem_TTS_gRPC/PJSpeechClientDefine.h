// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"

enum class EPJGenderType :uint8;

/**
 * TTS(Text-To-Speech) / STT(Speech-To-Text) 서비스에서 사용되는
 * 공통 열거형, 구조체, 콜백 타입 정의
 * - 요청/응답 상태 관리를 위한 State Machine 패턴 적용
 * - Lambda 콜백 기반 비동기 응답 처리
 */

// TTS/STT 응답 상태 - 스트리밍 응답의 진행 상태를 추적
enum class EPJSpeechResponseState:uint8
{
	None,
	Error,
	PartialResult,
	Finished,
};

// STT 응답 콜백: 음성→텍스트 변환 결과를 비동기로 전달
typedef TFunction<void(int64 RequestID, EPJSpeechResponseState ResponseState, const FString& Text)> FPJSTTResponseLambda;
// TTS 응답 콜백: 텍스트→음성 변환된 PCM 오디오 버퍼를 비동기로 전달
typedef TFunction<void(int64 RequestID, EPJSpeechResponseState ResponseState, const TArray<uint8>& PcmAudioBuffer)> FPJTTSResponseLambda;

// TTS/STT 요청 상태 - 스트리밍 요청의 라이프사이클 관리
enum class EPJSpeechRequestState :uint8
{
	None,
	Start,
	InProgress,
	End,
};

// TTS 클라이언트 타입 - Factory 패턴으로 런타임에 프로바이더 교체 가능
enum class EPJTTSClientType : uint8
{
	None,
	GeminiWebSocket,		// Google Gemini WebSocket 스트리밍
	MinimaxWebSocket,		// Minimax WebSocket 스트리밍
	GeminiLiveProHTTP,		// Google Gemini Live Pro HTTP
	GeminiLiveFlashHTTP,	// Google Gemini Live Flash HTTP
	CloudGrpc,				// Google Cloud TTS gRPC 스트리밍
};

struct FPJTTSRequestParams
{
	FString Text;
	int64 RequestID = 0;
	int32 SampleRate = 0;
	int32 VoiceModelId = 0;
	EPJGenderType Gender;
	EPJTTSClientType TTSClientType = EPJTTSClientType::None;
	EPJSpeechRequestState State = EPJSpeechRequestState::None;
	FPJTTSResponseLambda ResponseLambda = nullptr;

	void Reset()
	{
		Text.Empty();
		RequestID = 0;
		SampleRate = 0;
		VoiceModelId = 0;
		TTSClientType = EPJTTSClientType::None;
		State = EPJSpeechRequestState::None;
		ResponseLambda = nullptr;
	}
};


struct FPJSTTRequestParams
{
	int64 RequestID = 0;
	int32 SampleRate = 0;
	int32 ChannelCount = 0;
	TArray<uint8> PcmBuffer;
	FPJSTTResponseLambda ResponseLambda = nullptr;
	EPJSpeechRequestState State = EPJSpeechRequestState::None;

	void Reset()
	{
		RequestID = 0;
		SampleRate = 0;
		ChannelCount = 0;
		PcmBuffer.Empty();
		State = EPJSpeechRequestState::None;
		ResponseLambda = nullptr;
	}
};

