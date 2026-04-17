// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJSTTClientCloudGrpc.h"

DEFINE_LOG_CATEGORY(PJSTTClientCloudGrpc);

void UPJSTTClientCloudGrpc::Connect(const FString& InAuthToken)
{
	Super::Connect(InAuthToken);

	ReconnectService();
}

void UPJSTTClientCloudGrpc::Disconnect()
{
	STTService = nullptr;
}

bool UPJSTTClientCloudGrpc::IsConnected()
{
	return (ServiceState == EGrpcServiceState::Ready);
}

void UPJSTTClientCloudGrpc::ReconnectService()
{
	UGrpcServiceManager* GrpcManager = UGrpcUtilities::GetGrpcServiceManager(UPJNetworkManager::Get());
	if (STTService.Get())
	{
		GrpcManager->ReleaseService(STTService, true);
		STTService = nullptr;
	}

	GrpcManager->SetServiceEndPoint(TEXT("Speech"), TEXT("speech.googleapis.com:443"));
	STTService = Cast<USpeech>(GrpcManager->MakeService("Speech"));
	STTService->Connect();
	USpeechClient* STTClient = STTService->MakeClient();
	STTClient->OnStreamingRecognizeFinished.AddDynamic(this, &ThisClass::OnStreamingFinished);
	STTClient->OnStreamingRecognizeResponse.AddDynamic(this, &ThisClass::OnStreamingResponse);
	STTService->OnServiceStateChanged.AddDynamic(this, &ThisClass::OnServiceStateChanged);
	OnServiceStateChanged(STTService->GetServiceState());
}

void UPJSTTClientCloudGrpc::OnServiceStateChanged(EGrpcServiceState InServiceState)
{
	if (ServiceState != InServiceState)
	{
		ServiceState = InServiceState;
	}
}

void UPJSTTClientCloudGrpc::RequestSTT(const FPJSTTRequestParams& RequestParams)
{
	if (!STTService)
	{
		return;
	}

	if (RequestParams.State == EPJSpeechRequestState::Start)
	{
		STTService->InitStreamingRecognize(AuthToken);

		STTRequestID = RequestParams.RequestID;
		STTResponseLambda = RequestParams.ResponseLambda;

		// Config 전송: 인코딩, 샘플레이트, 언어 등 스트리밍 인식 설정
		FGrpcGoogleCloudSpeechV1StreamingRecognizeRequest configRequest;
		configRequest.Streaming_request.StreamingConfig = MakeShared<FGrpcGoogleCloudSpeechV1StreamingRecognitionConfig>();
		configRequest.Streaming_request.StreamingConfig->Config = MakeShared<FGrpcGoogleCloudSpeechV1RecognitionConfig>();
		configRequest.Streaming_request.StreamingConfig->Config->EnableAutomaticPunctuation = true;
		configRequest.Streaming_request.StreamingConfig->Config->DiarizationConfig = MakeShared<FGrpcGoogleCloudSpeechV1SpeakerDiarizationConfig>();
		configRequest.Streaming_request.StreamingConfig->Config->Metadata = MakeShared<FGrpcGoogleCloudSpeechV1RecognitionMetadata>();

		FGrpcGoogleCloudSpeechV1StreamingRecognitionConfig* StreamingConfig = configRequest.Streaming_request.StreamingConfig.Get();
		FGrpcGoogleCloudSpeechV1RecognitionConfig* RecognitionConfig = StreamingConfig->Config.Get();

		RecognitionConfig->DiarizationConfig = MakeShared<FGrpcGoogleCloudSpeechV1SpeakerDiarizationConfig>();
		RecognitionConfig->Encoding = EGrpcGoogleCloudSpeechV1RecognitionConfigAudioEncoding::LINEAR16;
		RecognitionConfig->SampleRateHertz = RequestParams.SampleRate;
		RecognitionConfig->AudioChannelCount = RequestParams.ChannelCount;
		RecognitionConfig->LanguageCode = TEXT("ko-KR");
		RecognitionConfig->Model = "telephony";
		RecognitionConfig->UseEnhanced = true;

		StreamingConfig->SingleUtterance = false;
		StreamingConfig->InterimResults = true;

		StreamingConfig->EnableVoiceActivityEvents = true;
		FGrpcGoogleCloudSpeechV1StreamingRecognitionConfigVoiceActivityTimeout VoiceActivityTimeout;
		VoiceActivityTimeout.SpeechEndTimeout.Seconds = 1.f;
		StreamingConfig->VoiceActivityTimeout = VoiceActivityTimeout;

		configRequest.Streaming_request.Streaming_requestCase = EGrpcGoogleCloudSpeechV1StreamingRecognizeRequestStreaming_request::StreamingConfig;

		STTService->CallStreamingRecognize(configRequest, nullptr);

		UE_LOG(PJSTTClientCloudGrpc, Log, TEXT("(RequestID:%d) Start"), STTRequestID);
	}
	else if (RequestParams.State == EPJSpeechRequestState::InProgress || RequestParams.State == EPJSpeechRequestState::End)
	{
		if (STTRequestID != 0)
		{
			FGrpcGoogleCloudSpeechV1StreamingRecognizeRequest audioRequest;
			audioRequest.Streaming_request.Streaming_requestCase = EGrpcGoogleCloudSpeechV1StreamingRecognizeRequestStreaming_request::AudioContent;
			if (RequestParams.State == EPJSpeechRequestState::InProgress)
			{
				audioRequest.Streaming_request.AudioContent.Value = RequestParams.PcmBuffer;
				UE_LOG(PJSTTClientCloudGrpc, Log, TEXT("(RequestID:%d) InProgress"), STTRequestID);
			}
			else
			{
				UE_LOG(PJSTTClientCloudGrpc, Log, TEXT("(RequestID:%d) End"), STTRequestID);
			}

			STTService->CallStreamingRecognize(audioRequest, nullptr);
		}
	}
}

void UPJSTTClientCloudGrpc::OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcGoogleCloudSpeechV1StreamingRecognizeResponse& Response)
{
	if (GrpcResult.Code == EGrpcResultCode::Ok && Response.Error.Code == 0)
	{
		for (auto Result : Response.Results)
		{
			for (auto Alternative : Result->Alternatives)
			{
				FString Transcript = Alternative->Transcript;
				if (Result->IsFinal)
				{
					STTRecognizeText += Transcript;
					if (STTResponseLambda.IsSet())
					{
						STTResponseLambda(STTRequestID, EPJSpeechResponseState::PartialResult, Transcript);
					}
					UE_LOG(PJSTTClientCloudGrpc, Log, TEXT("(RequestID:%d) PartialResult %s"), STTRequestID, *Transcript);
				}
			}
		}
	}
	else if (GrpcResult.Code != EGrpcResultCode::Ok)
	{
		UE_LOG(PJSTTClientCloudGrpc, Error, TEXT("(RequestID:%d) GrpcResult Error: ErrorCode [%s] Message [%s]"), STTRequestID, *GrpcResult.GetCodeString(), *GrpcResult.GetMessageString());
	}
	else if (Response.Error.Code != 0)
	{
		UE_LOG(PJSTTClientCloudGrpc, Error, TEXT("(RequestID:%d) Response Error: Code [%d] Message [%s]"), STTRequestID, Response.Error.Code, *Response.Error.Message);
	}
}

void UPJSTTClientCloudGrpc::OnStreamingFinished(const FGrpcResult& GrpcResult)
{
	bool bIsNoError = GrpcResult.Code == EGrpcResultCode::Ok || GrpcResult.Code == EGrpcResultCode::Cancelled;
	if (bIsNoError)
	{
		if (STTResponseLambda.IsSet())
		{
			STTResponseLambda(STTRequestID, EPJSpeechResponseState::Finished, STTRecognizeText);
		}
		UE_LOG(PJSTTClientCloudGrpc, Log, TEXT("(RequestID:%d) Finished"), STTRequestID);
	}
	else
	{
		if (STTResponseLambda.IsSet())
		{
			STTResponseLambda(STTRequestID, EPJSpeechResponseState::Error, STTRecognizeText);
		}
		UE_LOG(PJSTTClientCloudGrpc, Error, TEXT("(RequestID:%d) terminated abnormally: CodeString [%s] Message [%s]"), STTRequestID, *GrpcResult.GetCodeString(), *GrpcResult.Message);
	}

	if (!bIsNoError)
	{
		ReconnectService();
	}

	STTRequestID = 0;
	STTRecognizeText = TEXT("");
	STTResponseLambda.Reset();
}
