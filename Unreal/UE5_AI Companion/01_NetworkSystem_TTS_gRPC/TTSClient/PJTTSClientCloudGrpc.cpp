// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJTTSClientCloudGrpc.h"

DEFINE_LOG_CATEGORY(PJTTSClientCloudGrpc);


void UPJTTSClientCloudGrpc::Connect(const FString& InAuthToken)
{
	Super::Connect(InAuthToken);

	UGrpcServiceManager* GrpcManager = UGrpcUtilities::GetGrpcServiceManager(UPJNetworkManager::Get());
	if (TTSService.Get())
	{
		GrpcManager->ReleaseService(TTSService, true);
		TTSService = nullptr;
	}

	// TTS 서비스 및 클라이언트 생성
	GrpcManager->SetServiceEndPoint(TEXT("TextToSpeech"), TEXT("texttospeech.googleapis.com:443"));
	TTSService = Cast<UTextToSpeech>(GrpcManager->MakeService("TextToSpeech"));
	TTSService->Connect();
	UTextToSpeechClient* TTSClient = TTSService->MakeClient();
	TTSClient->OnSpeechStreamingSynthesizeFinished.AddDynamic(this, &ThisClass::OnStreamingFinished);
	TTSClient->OnStreamingSynthesizeResponse.AddDynamic(this, &ThisClass::OnStreamingResponse);
	TTSService->OnServiceStateChanged.AddDynamic(this, &ThisClass::OnServiceStateChanged);
	OnServiceStateChanged(TTSService->GetServiceState());
}

void UPJTTSClientCloudGrpc::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	Super::RequestTTS(RequestParams);

	FGrpcGoogleCloudTexttospeechV1StreamingSynthesizeRequest Request;
	FGrpcGoogleCloudTexttospeechV1StreamingSynthesizeRequestStreaming_request& StreamingRequest = Request.Streaming_request;

	if (RequestParams.State == EPJSpeechRequestState::Start)
	{
		TTSRequestID = RequestParams.RequestID;
		TTSResponseLambda = RequestParams.ResponseLambda;
		TTSService->InitStreamingSynthesize(AuthToken);

		StreamingRequest.Streaming_requestCase = EGrpcGoogleCloudTexttospeechV1StreamingSynthesizeRequestStreaming_request::StreamingConfig;

		FGrpcGoogleCloudTexttospeechV1StreamingSynthesizeConfig& StreamingConfig = StreamingRequest.StreamingConfig;
		StreamingConfig.Voice.LanguageCode = VoiceLanguage;
		StreamingConfig.Voice.Name = VoiceName;
		StreamingConfig.Voice.SsmlGender = EGrpcGoogleCloudTexttospeechV1SsmlVoiceGender::NEUTRAL;
		// 압축 포맷은 스트리밍으로 다 받은 다음에 디코딩을 해야 해서 스트리밍의 의미가 없음. 그리고 OGG_OPUS는 sampleRateHertz 48000으로 설정해야함.
		StreamingConfig.StreamingAudioConfig.AudioEncoding = EGrpcGoogleCloudTexttospeechV1AudioEncoding::PCM;//EGrpcGoogleCloudTexttospeechV1AudioEncoding::OGG_OPUS;
		StreamingConfig.StreamingAudioConfig.SampleRateHertz = RequestParams.SampleRate;

		TTSService->CallStreamingSynthesize(Request, nullptr);

		UE_LOG(PJTTSClientCloudGrpc, Log, TEXT("(RequestID:%d) Start "), TTSRequestID);
	}
	else if (RequestParams.State == EPJSpeechRequestState::InProgress || RequestParams.State == EPJSpeechRequestState::End)
	{
		if (TTSRequestID != 0)
		{
			StreamingRequest.Streaming_requestCase = EGrpcGoogleCloudTexttospeechV1StreamingSynthesizeRequestStreaming_request::Input;
			FGrpcGoogleCloudTexttospeechV1StreamingSynthesisInput& Input = StreamingRequest.Input;

			Input.Input_source.Input_sourceCase = EGrpcGoogleCloudTexttospeechV1StreamingSynthesisInputInput_source::Text;
			if (RequestParams.State == EPJSpeechRequestState::InProgress)
			{
				Input.Input_source.Text = RequestParams.Text;
				UE_LOG(PJTTSClientCloudGrpc, Log, TEXT("(RequestID:%d) InProgress "), TTSRequestID);
			}
			else
			{
				// 비어 있는 Text를 보내서 context에서 set_last_message옵션으로 write를 한다.
				Input.Input_source.Text = TEXT("");
				UE_LOG(PJTTSClientCloudGrpc, Log, TEXT("(RequestID:%d) End "), TTSRequestID);
			}

			TTSService->CallStreamingSynthesize(Request, nullptr);
		}
	}
}

void UPJTTSClientCloudGrpc::OnServiceStateChanged(EGrpcServiceState InServiceState)
{
	if (ServiceState != InServiceState)
	{
		ServiceState = InServiceState;
	}
}


void UPJTTSClientCloudGrpc::OnStreamingResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcGoogleCloudTexttospeechV1StreamingSynthesizeResponse& Response)
{
	if (GrpcResult.Code == EGrpcResultCode::Ok)
	{
		TArray<uint8> AudioBuffer = Response.AudioContent.Value;
		TTSSynthesizeFinalAudioBuffer.Append(AudioBuffer);
		if (TTSResponseLambda.IsSet())
		{
			TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::PartialResult, AudioBuffer);
		}
	}
	else if (GrpcResult.Code != EGrpcResultCode::Ok)
	{
		UE_LOG(PJTTSClientCloudGrpc, Error, TEXT("(RequestID:%d) GrpcResult Error: ErrorCode [%s]  Message [%s] "), TTSRequestID, *GrpcResult.GetCodeString(), *GrpcResult.GetMessageString());
	}
}

// 스트리밍 종료 콜백
void UPJTTSClientCloudGrpc::OnStreamingFinished(const FGrpcResult& GrpcResult)
{
	// 정상 종료.
	if (GrpcResult.Code == EGrpcResultCode::Ok)
	{
		// 스트리밍 종료가 되면 분석된 텍스트를 보낸다.
		if (TTSResponseLambda.IsSet())
		{
			TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Finished, TTSSynthesizeFinalAudioBuffer);
		}
		UE_LOG(PJTTSClientCloudGrpc, Log, TEXT("(RequestID:%d) Finished "), TTSRequestID);
	}
	// 비정상 정료
	else
	{
		// 에러 처리
		if (TTSResponseLambda.IsSet())
		{
			TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Error, TTSSynthesizeFinalAudioBuffer);
		}
		UE_LOG(PJTTSClientCloudGrpc, Error, TEXT("(RequestID:%d) terminated abnormally: %s "), TTSRequestID, *GrpcResult.Message);
	}

	TTSRequestID = 0;
	TTSResponseLambda.Reset();
	TTSSynthesizeFinalAudioBuffer.Empty();
}