// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJTTSClientGeminiLiveHTTP.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"

DEFINE_LOG_CATEGORY(PJTTSClientGeminiLiveHTTP);

void UPJTTSClientGeminiLiveHTTP::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	Super::RequestTTS(RequestParams);

	if (RequestParams.State != EPJSpeechRequestState::InProgress)
	{
		if (RequestParams.State == EPJSpeechRequestState::Start)
		{
			TTSRequestID = RequestParams.RequestID;
			TTSResponseLambda = RequestParams.ResponseLambda;
		}
		return;
	}

	FString TTSLLM = (GetClientType() == EPJTTSClientType::GeminiLiveProHTTP) ? TEXT("gemini-2.5-pro-preview-tts") : TEXT("gemini-2.5-flash-preview-tts");
	// REST 엔드포인트 URL
	const FString Url = FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent"), *TTSLLM);

	// 요청 본문 JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	// EndPoint에 모델 정��가 있기 때문에 안보내도 됨.
	TArray<TSharedPtr<FJsonValue>> ContentsArray;
	TSharedPtr<FJsonObject> ContentObj = MakeShared<FJsonObject>();
	ContentObj->SetStringField(TEXT("role"), TEXT("user"));

	TArray<TSharedPtr<FJsonValue>> PartsArray;
	TSharedPtr<FJsonObject> PartObj = MakeShared<FJsonObject>();
	PartObj->SetStringField(TEXT("text"), RequestParams.Text);
	PartsArray.Add(MakeShared<FJsonValueObject>(PartObj));

	ContentObj->SetArrayField(TEXT("parts"), PartsArray);
	ContentsArray.Add(MakeShared<FJsonValueObject>(ContentObj));

	JsonObject->SetArrayField(TEXT("contents"), ContentsArray);

	// generationConfig
	TSharedPtr<FJsonObject> GenerationConfig = MakeShared<FJsonObject>();

	// responseModalities
	TArray<TSharedPtr<FJsonValue>> Modalities;
	Modalities.Add(MakeShared<FJsonValueString>(TEXT("AUDIO")));
	GenerationConfig->SetArrayField(TEXT("responseModalities"), Modalities);

	// voiceConfig
	TSharedPtr<FJsonObject> VoiceConfig = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> PrebuiltVoiceConfig = MakeShared<FJsonObject>();
	PrebuiltVoiceConfig->SetStringField(TEXT("voiceName"), VoiceName);	// https://ai.google.dev/gemini-api/docs/speech-generation?utm_source=chatgpt.com&hl=ko
	VoiceConfig->SetObjectField(TEXT("prebuiltVoiceConfig"), PrebuiltVoiceConfig);

	// audioConfig 지원 안함 : Encoding,SampleRate 는 LINEAR16 24000으로 고정 됨.

	// speechConfig에 voiceConfig 추가
	TSharedPtr<FJsonObject> SpeechConfig = MakeShared<FJsonObject>();
	SpeechConfig->SetObjectField(TEXT("voiceConfig"), VoiceConfig);

	GenerationConfig->SetObjectField(TEXT("speechConfig"), SpeechConfig);

	// attach generationConfig
	JsonObject->SetObjectField(TEXT("generationConfig"), GenerationConfig);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(60.f);
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpRequest->SetContentAsString(RequestBody);

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnResponse);

	HttpRequest->ProcessRequest();

	UE_LOG(PJTTSClientGeminiLiveHTTP, Log, TEXT("(RequestID:%d) request send "), RequestParams.RequestID);
}


void UPJTTSClientGeminiLiveHTTP::OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		// 에러 처리
		if (TTSResponseLambda.IsSet())
		{
			TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Error, TTSSynthesizeFinalAudioBuffer);
		}

		if (!bWasSuccessful && !Response.IsValid())
		{
			UE_LOG(PJTTSClientGeminiLiveHTTP, Error, TEXT("(RequestID:%d) response timeout "), TTSRequestID);
		}
		else
		{
			UE_LOG(PJTTSClientGeminiLiveHTTP, Error, TEXT("(RequestID:%d) response failed "), TTSRequestID);
		}
	}
	else
	{
		FString ResponseString = Response->GetContentAsString();
		FString Base64AudioData = GetAudioDataFromGeminiTTSResponse(ResponseString);
		// 오디오 데이터가 없으면 에러 처리
		if (Base64AudioData.IsEmpty())
		{
			if (TTSResponseLambda.IsSet())
			{
				TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Error, TTSSynthesizeFinalAudioBuffer);
			}
			UE_LOG(PJTTSClientGeminiLiveHTTP, Error, TEXT("(RequestID:%d) response failed to convert to audio "), TTSRequestID);
		}
		// 있으면 Base64 -> L16으로 변환
		else
		{
			if (FBase64::Decode(Base64AudioData, TTSSynthesizeFinalAudioBuffer))
			{
				if (TTSResponseLambda.IsSet())
				{
					TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::PartialResult, TTSSynthesizeFinalAudioBuffer);
				}

				if (TTSResponseLambda.IsSet())
				{
					TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Finished, TTSSynthesizeFinalAudioBuffer);
				}
				UE_LOG(PJTTSClientGeminiLiveHTTP, Log, TEXT("(RequestID:%d) response successed "), TTSRequestID);
			}
			else
			{
				if (TTSResponseLambda.IsSet())
				{
					TTSResponseLambda(TTSRequestID, EPJSpeechResponseState::Error, TTSSynthesizeFinalAudioBuffer);
				}
				UE_LOG(PJTTSClientGeminiLiveHTTP, Error, TEXT("(RequestID:%d) response failed to convert response to Base64 audio "), TTSRequestID);
			}
		}
	}

	TTSRequestID = 0;
	TTSResponseLambda.Reset();
	TTSSynthesizeFinalAudioBuffer.Empty();
}


FString UPJTTSClientGeminiLiveHTTP::GetAudioDataFromGeminiTTSResponse(const FString& ResponseString)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return TEXT("");

	const TArray<TSharedPtr<FJsonValue>>* candidates;
	if (!Root->TryGetArrayField(TEXT("candidates"), candidates))
	{
		return TEXT("");
	}

	int32 DataCount = 0;

	FString AudioBase64 = TEXT("");
	for (TSharedPtr<FJsonValue> Candidate : *candidates)
	{
		TSharedPtr<FJsonObject> CandidateObj = Candidate->AsObject();
		if (CandidateObj.IsValid())
		{
			const TSharedPtr<FJsonObject>* ContentObj;
			if (CandidateObj->TryGetObjectField(TEXT("content"), ContentObj))
			{
				const TArray<TSharedPtr<FJsonValue>>* Parts;
				if (ContentObj->Get()->TryGetArrayField(TEXT("parts"), Parts))
				{
					for (TSharedPtr<FJsonValue> Part : *Parts)
					{
						const TSharedPtr<FJsonObject>* InlineDataObj;
						if (Part->AsObject()->TryGetObjectField(TEXT("inlineData"), InlineDataObj))
						{
							FString AudioData;
							if (InlineDataObj->Get()->TryGetStringField(TEXT("data"), AudioData))
							{
								AudioBase64 += AudioData;
								DataCount++;
							}
						}
					}
				}
			}
			FString FinishReason;
			if (CandidateObj->TryGetStringField(TEXT("finishReason"), FinishReason))
			{
				// GeminiLive에서 필터 걸려서 TTS 처리가 안 된 경우... 로그 출력. 간혹 동일한 걸 보내도 이렇게 오류가 오는 경우가 있다.
				if (!FinishReason.Equals(TEXT("STOP"), ESearchCase::IgnoreCase))
				{
					UE_LOG(PJTTSClientGeminiLiveHTTP, Error, TEXT("(RequestID:%d)  Response Error - Finish Reason [%s]"), TTSRequestID, *FinishReason);
				}
			}
		}
	}

	if (DataCount > 1)
	{
		UE_LOG(PJTTSClientGeminiLiveHTTP, Log, TEXT("(RequestID:%d) received multiple parts "), TTSRequestID);
	}

	return AudioBase64;
}