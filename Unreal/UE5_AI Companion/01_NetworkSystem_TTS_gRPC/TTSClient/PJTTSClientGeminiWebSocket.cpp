// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJTTSClientGeminiWebSocket.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"


DEFINE_LOG_CATEGORY(PJTTSClientGeminiWebSocket);


void UPJTTSClientGeminiWebSocket::Connect(const FString& InAuthToken)
{
	AuthToken = InAuthToken;
	Connect();
}


void UPJTTSClientGeminiWebSocket::Connect()
{
	if (WebSocket)
	{
		Disconnect();
	}

	TMap<FString, FString> Headers;
	Headers.Add(TEXT("authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));

	FString EndPoint = FString::Printf(TEXT("wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.%s.GenerativeService.BidiGenerateContent"), *APIVersion);
	// WebSocket 생성
	WebSocket = FWebSocketsModule::Get().CreateWebSocket(
		EndPoint,//TEXT("wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent"),
		TEXT(""),
		Headers
	);

	WebSocket->OnConnected().AddUObject(this, &ThisClass::OnConnected);
	WebSocket->OnMessage().AddUObject(this, &ThisClass::OnMessage);
	WebSocket->OnRawMessage().AddUObject(this, &ThisClass::OnRawMessage);
	WebSocket->OnBinaryMessage().AddUObject(this, &ThisClass::OnBinaryMessage);
	WebSocket->OnConnectionError().AddUObject(this, &ThisClass::OnConnectionError);
	WebSocket->OnClosed().AddUObject(this, &ThisClass::OnClosed);

	WebSocket->Connect();
	UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("Websocket Connect Request"));
}

void UPJTTSClientGeminiWebSocket::Disconnect()
{
	if(!WebSocket)
		return;

	WebSocket->OnConnected().RemoveAll(this);
	WebSocket->OnMessage().RemoveAll(this);
	WebSocket->OnRawMessage().RemoveAll(this);
	WebSocket->OnBinaryMessage().RemoveAll(this);
	WebSocket->OnConnectionError().RemoveAll(this);
	WebSocket->OnClosed().RemoveAll(this);

	if (WebSocket->IsConnected() && !bCloseRequested)
	{
		bCloseRequested = true;

		FString Reason = TEXT("shutting down");

		UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("Closing websocket "));
		WebSocket->Close(1000, Reason);
		OnClosed(1000, Reason, true);
	}

	WebSocket = nullptr;
}


void UPJTTSClientGeminiWebSocket::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	Super::RequestTTS(RequestParams);

	// VoiceLanguage, VoiceName이 변경되면 다시 Connect를 하고 Setup을 한다.
	if (WebSocket == nullptr || !(VoiceName.Equals(CurrentVoiceName)) )
	{
		CurrentVoiceName = VoiceName;
		BackupRequestParams = RequestParams;
		Connect();
	}

	if (!WebSocket->IsConnected())
		return;

	if (RequestParams.State == EPJSpeechRequestState::InProgress)
	{
		ResetTTSInfo();
		TTSRequestID = RequestParams.RequestID;
		TTSResponseLambda = RequestParams.ResponseLambda;
		TTSSampleRate = RequestParams.SampleRate;

		FString ClientContentRequest;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ClientContentRequest);
		TSharedPtr<FJsonObject> JsonObject = MakeClientContentJsonObject(RequestParams.Text);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		WebSocket->Send(ClientContentRequest);

		UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("WebSocket clientContent send"));
	}
}


TSharedPtr<FJsonObject> UPJTTSClientGeminiWebSocket::MakeSetupJsonObject()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	{
		TSharedPtr<FJsonObject> Setup = MakeShared<FJsonObject>();
		{
			TSharedPtr<FJsonObject> GenerationConfig = MakeShared<FJsonObject>();
			{
				TSharedPtr<FJsonObject> SpeechConfig = MakeShared<FJsonObject>();
				{
					TSharedPtr<FJsonObject> VoiceConfig = MakeShared<FJsonObject>();
					{
						TSharedPtr<FJsonObject> PrebuiltVoiceConfig = MakeShared<FJsonObject>();
						{
							PrebuiltVoiceConfig->SetStringField(TEXT("voiceName"), CurrentVoiceName);
							VoiceConfig->SetObjectField(TEXT("prebuiltVoiceConfig"), PrebuiltVoiceConfig);
						}
					}
					SpeechConfig->SetObjectField(TEXT("voiceConfig"), VoiceConfig);
					SpeechConfig->SetStringField(TEXT("languageCode"), VoiceLanguage);
				}

				GenerationConfig->SetObjectField(TEXT("speechConfig"), SpeechConfig);
				TArray<TSharedPtr<FJsonValue>> ResponseModalities;
				{
					ResponseModalities.Add(MakeShared<FJsonValueString>(TEXT("AUDIO")));
					GenerationConfig->SetArrayField(TEXT("responseModalities"), ResponseModalities);
				}
			}

			Setup->SetStringField(TEXT("model"), LLMModelName);
			Setup->SetObjectField(TEXT("generationConfig"), GenerationConfig);

			TSharedPtr<FJsonObject> SystemInstructionObj = MakeRolePartsJsonObject(TEXT("system"), SystemInstruction);
			Setup->SetObjectField(TEXT("systemInstruction"), SystemInstructionObj);
		}

		JsonObject->SetObjectField(TEXT("setup"), Setup);
	}

	return JsonObject;
}

TSharedPtr<FJsonObject> UPJTTSClientGeminiWebSocket::MakeClientContentJsonObject(const FString& Message)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	{
		TSharedPtr<FJsonObject> ClientContent = MakeShared<FJsonObject>();
		{
			FString Text = MessageInstruction + Message;
			TSharedPtr<FJsonObject> Turns = MakeRolePartsJsonObject(TEXT("user"), Text);
			ClientContent->SetObjectField(TEXT("turns"), Turns);
			ClientContent->SetBoolField(TEXT("turnComplete"), true);
		}
		JsonObject->SetObjectField(TEXT("clientContent"), ClientContent);
	}

	return JsonObject;
}

TSharedPtr<FJsonObject> UPJTTSClientGeminiWebSocket::MakeRolePartsJsonObject(const FString& Role, const FString& Text)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	// role
	JsonObject->SetStringField(TEXT("role"), Role);
	// parts 배열
	TArray<TSharedPtr<FJsonValue>> PartsArray;
	{
		TSharedPtr<FJsonObject> PartObj = MakeShared<FJsonObject>();
		PartObj->SetStringField(TEXT("text"), Text);

		PartsArray.Add(MakeShared<FJsonValueObject>(PartObj));
	}

	JsonObject->SetArrayField(TEXT("parts"), PartsArray);
	return JsonObject;
}

void UPJTTSClientGeminiWebSocket::OnConnected()
{
	bCloseRequested = false;

	// 컨넥션 연결 되면 Setup
	FString SetupRequest;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SetupRequest);
	TSharedPtr<FJsonObject> JsonObject = MakeSetupJsonObject();
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	WebSocket->Send(SetupRequest);

	UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("WebSocket connected And Setup"));

	if (BackupRequestParams.RequestID != 0)
	{
		BackupRequestParams.State = EPJSpeechRequestState::InProgress;
		RequestTTS(BackupRequestParams);
	}
}

// Gemini는 바이너리 메시지(OnBinaryMessage)만 사용하므로 텍스트/Raw 콜백은 사용하지 않음
void UPJTTSClientGeminiWebSocket::OnMessage(const FString& Message) {}
void UPJTTSClientGeminiWebSocket::OnRawMessage(const void* Data, SIZE_T Size, SIZE_T BytesRemaining) {}


void UPJTTSClientGeminiWebSocket::OnBinaryMessage(const void* Data, SIZE_T Size, bool bIsLastFragment)
{
	UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("OnBinaryMessage bIsLastFragment %d"), bIsLastFragment);

	FString JsonStr;
	FFileHelper::BufferToString(JsonStr, reinterpret_cast<const uint8*>(Data), Size);

	CurrentJsonMessage += JsonStr;

	if (bIsLastFragment)
	{
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CurrentJsonMessage);
		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
		{
			const TSharedPtr<FJsonObject>* SetupComplete;
			if (Root->TryGetObjectField(TEXT("setupComplete"), SetupComplete))
			{
				UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("SetupComplete"));
			}

			const TSharedPtr<FJsonObject>* ServerContent;
			if (Root->TryGetObjectField(TEXT("serverContent"), ServerContent))
			{
				bool bTurnComplete = false;
				ServerContent->Get()->TryGetBoolField(TEXT("turnComplete"), bTurnComplete);
				// 응답 종료
				if (bTurnComplete)
				{
					UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("TurnComplete"));
				}
				else
				{
					TArray<uint8> AudioBuffer;
					bool bGenerationComplete = false;
					FString Base64AudioData = GetAudioDataFromServerContent(ServerContent, bGenerationComplete);
					if (bGenerationComplete)
					{
						FBase64::Decode(Base64AudioData, AudioBuffer);
						if (!AudioBuffer.IsEmpty())
						{
							TTSSynthesizePartialBuffer.Append(AudioBuffer);
						}
						// 종료 전에 남은게 있으면 PartialResult
						if (!TTSSynthesizePartialBuffer.IsEmpty())
						{
							OnTTSResponse(EPJSpeechResponseState::PartialResult);
						}
						OnTTSResponse(EPJSpeechResponseState::Finished);
					}
					else
					{
						// 오디오 데이터가 있으면 Base64 -> L16으로 변환
						FBase64::Decode(Base64AudioData, AudioBuffer);
						if (!AudioBuffer.IsEmpty())
						{
							TTSSynthesizePartialBuffer.Append(AudioBuffer);
							if (TTSSynthesizeFinalAudioBuffer.IsEmpty())
							{
								// 오디오 플레이 끊김 현상을 개선하기 위하여 PreBufferDurationSec 분량이 될 때까지 모은 다음 플레이 시작한다.
								int32 bufferSize = TTSSampleRate * PreBufferDurationSec * 2; // L16이라 2를 곱함.
								if (TTSSynthesizePartialBuffer.Num() > bufferSize)
								{
									OnTTSResponse(EPJSpeechResponseState::PartialResult);
								}
							}
							else
							{
								OnTTSResponse(EPJSpeechResponseState::PartialResult);
							}
						}
					}
				}
			}
			CurrentJsonMessage.Empty();
		}
	}	
}

void UPJTTSClientGeminiWebSocket::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("OnClosed: %s"), *Reason);
	WebSocket = nullptr;
	ResetTTSInfo();
}
void UPJTTSClientGeminiWebSocket::OnConnectionError(const FString& Reason)
{
	WebSocket = nullptr;
	ResetTTSInfo();
	UE_LOG(PJTTSClientGeminiWebSocket, Error, TEXT("OnConnectionError Reason : %s"), *Reason);
}



void UPJTTSClientGeminiWebSocket::OnTTSResponse(EPJSpeechResponseState ResponseState)
{
	TArray<uint8> AudioBuffer;
	if (ResponseState == EPJSpeechResponseState::PartialResult)
	{
		AudioBuffer = TTSSynthesizePartialBuffer;
		TTSSynthesizeFinalAudioBuffer.Append(AudioBuffer);
		TTSSynthesizePartialBuffer.Empty();
		UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("(RequestID:%d) PartialResult "), TTSRequestID);
	}
	else if (ResponseState == EPJSpeechResponseState::Finished)
	{
		AudioBuffer = TTSSynthesizeFinalAudioBuffer;
		TTSSynthesizeFinalAudioBuffer.Empty();
		UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("(RequestID:%d) Finished "), TTSRequestID);
	}

	if (TTSResponseLambda.IsSet())
	{
		TTSResponseLambda(TTSRequestID, ResponseState, AudioBuffer);
	}

	if (ResponseState != EPJSpeechResponseState::PartialResult)
	{
		ResetTTSInfo();
	}
}

void UPJTTSClientGeminiWebSocket::ResetTTSInfo()
{
	BackupRequestParams.RequestID = 0;
	TTSRequestID = 0;
	TTSResponseLambda.Reset();
	TTSSynthesizeFinalAudioBuffer.Empty();
	TTSSynthesizePartialBuffer.Empty();
	CurrentJsonMessage.Empty();
}

FString UPJTTSClientGeminiWebSocket::GetAudioDataFromServerContent(const TSharedPtr<FJsonObject>* ServerContent, bool& bGenerationComplete)
{
	// TTS 완료 여부.
	bGenerationComplete = false;
	ServerContent->Get()->TryGetBoolField(TEXT("generationComplete"), bGenerationComplete);

	int32 AudioDataCount = 0;
	FString Base64AudioData = TEXT("");

	const TSharedPtr<FJsonObject>* ModelTurn;
	if (ServerContent->Get()->TryGetObjectField(TEXT("modelTurn"), ModelTurn))
	{
		const TArray<TSharedPtr<FJsonValue>>* Parts;
		if (ModelTurn->Get()->TryGetArrayField(TEXT("parts"), Parts))
		{
			for (TSharedPtr<FJsonValue> Part : *Parts)
			{
				const TSharedPtr<FJsonObject>* InlineDataObj;
				if (Part->AsObject()->TryGetObjectField(TEXT("inlineData"), InlineDataObj))
				{
					FString MimeType;
					if (InlineDataObj->Get()->TryGetStringField(TEXT("mimeType"), MimeType))
					{
						//"audio/pcm;rate=24000"
					}

					FString Data;
					if (InlineDataObj->Get()->TryGetStringField(TEXT("data"), Data))
					{
						Base64AudioData += Data;
						AudioDataCount++;
					}
				}
			}
		}
	}

	if (AudioDataCount > 1)
	{
		UE_LOG(PJTTSClientGeminiWebSocket, Log, TEXT("(RequestID:%d) received multiple parts "), TTSRequestID);
	}

	return Base64AudioData;
}

