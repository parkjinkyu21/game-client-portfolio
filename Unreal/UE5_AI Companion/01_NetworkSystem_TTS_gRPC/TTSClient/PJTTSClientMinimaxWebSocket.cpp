// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJTTSClientMinimaxWebSocket.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"


DEFINE_LOG_CATEGORY(PJTTSClientMinimaxWebSocket);


void UPJTTSClientMinimaxWebSocket::Connect(const FString& InAuthToken)
{
	AuthToken = InAuthToken;
	Connect();
}


void UPJTTSClientMinimaxWebSocket::Connect()
{
	if (WebSocket)
	{
		Disconnect();
	}



	TMap<FString, FString> Headers;
	Headers.Add(TEXT("authorization"), FString::Printf(TEXT("Bearer %s"), *APIKey));

	FString EndPoint = FString::Printf(TEXT("wss://api.minimax.io/ws/v1/t2a_v2?groupId=%s"), *GroupID);
	// WebSocket 생성
	WebSocket = FWebSocketsModule::Get().CreateWebSocket(EndPoint, TEXT(""), Headers);

	WebSocket->OnConnected().AddUObject(this, &ThisClass::OnConnected);
	WebSocket->OnMessage().AddUObject(this, &ThisClass::OnMessage);
	WebSocket->OnRawMessage().AddUObject(this, &ThisClass::OnRawMessage);
	WebSocket->OnBinaryMessage().AddUObject(this, &ThisClass::OnBinaryMessage);
	WebSocket->OnConnectionError().AddUObject(this, &ThisClass::OnConnectionError);
	WebSocket->OnClosed().AddUObject(this, &ThisClass::OnClosed);

	WebSocket->Connect();
	UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("Websocket Connect Request"));
}

void UPJTTSClientMinimaxWebSocket::Disconnect()
{
	if(!WebSocket)
		return;

	ResetTTSInfo();

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

		UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("Closing websocket "));
		WebSocket->Close(1000, Reason);
		OnClosed(1000, Reason, true);
	}

	WebSocket = nullptr;
}


void UPJTTSClientMinimaxWebSocket::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	Super::RequestTTS(RequestParams);

	// Minimax는 연결 제한시간이 120라서 TTS요청 한번당 Connection 한번씩 처리해야 한다. TTS 완료시 DisConnenct를 한다.
	if (RequestParams.State == EPJSpeechRequestState::Start)
	{
		// 이전 처리중인 TTS가 있으면 종료
		if (BackupRequestParams.State != EPJSpeechRequestState::None)
		{
			OnTTSResponse(EPJSpeechResponseState::Finished);
		}

		if (WebSocket)
		{
			Disconnect();
		}

		Connect();
		BackupRequestParams = RequestParams;
	}
}

void UPJTTSClientMinimaxWebSocket::OnConnected()
{
	bCloseRequested = false;
}

void UPJTTSClientMinimaxWebSocket::OnMessage(const FString& Message)
{
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FString EventStr;
		JsonObject->TryGetStringField(TEXT("event"), EventStr);
		if (EventStr.Equals(TEXT("connected_success")))
		{
			// TTS 세팅
			SendTaskStart();
		}
		else if (EventStr.Equals(TEXT("task_started")))
		{
			// TTS 요청
			SendContinue();
		}
		else if (EventStr.Equals(TEXT("task_continued")))
		{
			// 오디오 처리
			ReceiveContinued(JsonObject);

			bool bIsFinal = false;
			if (JsonObject->TryGetBoolField(TEXT("is_final"), bIsFinal) && bIsFinal == true)
			{
				SendFinish();
			}
		}
		else if (EventStr.Equals(TEXT("task_finished")))
		{
			// 연결 종료.
			Disconnect();
		}
		else if (EventStr.Equals(TEXT("task_failed")))
		{
			int32 StatusCode = 0;
			FString StatusMsg;
			const TSharedPtr<FJsonObject>* BaseResp;
			if (JsonObject->TryGetObjectField(TEXT("base_resp"), BaseResp))
			{
				BaseResp->Get()->TryGetNumberField(TEXT("status_code"), StatusCode);
				BaseResp->Get()->TryGetStringField(TEXT("status_msg"), StatusMsg);
			}
			UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("task_failed: %s, Code: %d, Msg: %s"), *EventStr, StatusCode, *StatusMsg);
			// 실패 및 종료.
			OnTTSResponse(EPJSpeechResponseState::Error);
			Disconnect();
		}		
	}

}

void UPJTTSClientMinimaxWebSocket::OnRawMessage(const void* Data, SIZE_T Size, SIZE_T BytesRemaining)
{
}


void UPJTTSClientMinimaxWebSocket::OnBinaryMessage(const void* Data, SIZE_T Size, bool bIsLastFragment)
{
}

void UPJTTSClientMinimaxWebSocket::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("OnClosed: %s"), *Reason);
	WebSocket = nullptr;
	ResetTTSInfo();
}
void UPJTTSClientMinimaxWebSocket::OnConnectionError(const FString& Reason)
{
	WebSocket = nullptr;
	OnTTSResponse(EPJSpeechResponseState::Error);
	ResetTTSInfo();
	UE_LOG(PJTTSClientMinimaxWebSocket, Error, TEXT("OnConnectionError Reason : %s"), *Reason);
}


void UPJTTSClientMinimaxWebSocket::SendTaskStart()
{
	FString VoiceId = VoiceName;
	FString Emotion = TEXT("");

	TSharedPtr<FJsonObject> RequestObject = MakeShared<FJsonObject>();
	RequestObject->SetStringField(TEXT("event"), TEXT("task_start"));
	RequestObject->SetStringField(TEXT("model"), *LLMModelName);

	// voice_setting
	TSharedPtr<FJsonObject> VoiceSetting = MakeShared<FJsonObject>();
	VoiceSetting->SetStringField(TEXT("voice_id"), *VoiceId);
	VoiceSetting->SetNumberField(TEXT("speed"), VoiceSpeed);
	VoiceSetting->SetNumberField(TEXT("vol"), VoiceVolum);
	VoiceSetting->SetNumberField(TEXT("pitch"), VoicePitch);
	if (!Emotion.IsEmpty())
	{
		VoiceSetting->SetStringField(TEXT("emotion"), *Emotion);
	}

	RequestObject->SetObjectField(TEXT("voice_setting"), VoiceSetting);

	// audio_setting
	TSharedPtr<FJsonObject> AudioSetting = MakeShared<FJsonObject>();
	AudioSetting->SetStringField(TEXT("format"), TEXT("pcm"));
	AudioSetting->SetNumberField(TEXT("sample_rate"), BackupRequestParams.SampleRate);
	AudioSetting->SetNumberField(TEXT("bitrate"), 64000);
	AudioSetting->SetNumberField(TEXT("channel"), 1);
	RequestObject->SetObjectField(TEXT("audio_setting"), AudioSetting);

	FString RequestJsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestJsonString);
	FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer);
	WebSocket->Send(RequestJsonString);

	UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("SendTaskStart"));
}

void UPJTTSClientMinimaxWebSocket::SendContinue()
{
	TSharedPtr<FJsonObject> RequestObject = MakeShared<FJsonObject>();
	RequestObject->SetStringField(TEXT("event"), TEXT("task_continue"));
	RequestObject->SetStringField(TEXT("text"), *BackupRequestParams.Text);

	FString RequestJsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestJsonString);
	FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer);
	WebSocket->Send(RequestJsonString);

	UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("SendContinue"));
}

void UPJTTSClientMinimaxWebSocket::SendFinish()
{
	TSharedPtr<FJsonObject> RequestObject = MakeShared<FJsonObject>();
	RequestObject->SetStringField(TEXT("event"), TEXT("task_finish"));

	FString RequestJsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestJsonString);
	FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer);
	WebSocket->Send(RequestJsonString);

	OnTTSResponse(EPJSpeechResponseState::Finished);

	UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("SendFinish"));
}

void UPJTTSClientMinimaxWebSocket::ReceiveContinued(const TSharedPtr<FJsonObject>& JsonObject)
{
	const TSharedPtr<FJsonObject>* DataObj;
	if (JsonObject->TryGetObjectField(TEXT("data"), DataObj))
	{
		FString HexAudioData;
		DataObj->Get()->TryGetStringField(TEXT("audio"), HexAudioData);

		TArray<uint8> AudioBuffer;
		AudioBuffer.AddUninitialized(HexAudioData.Len() * 0.5);
		if (UE::String::HexToBytes(HexAudioData, AudioBuffer.GetData()) > 0)
		{
			TTSSynthesizePartialBuffer.Append(AudioBuffer);
			if (TTSSynthesizeFinalAudioBuffer.IsEmpty())
			{
				// 오디오 플레이 끊김 현상을 개선하기 위하여 PreBufferDurationSec 분량이 될 때까지 모은 다음 플레이 시작한다.
				int32 bufferSize = BackupRequestParams.SampleRate * PreBufferDurationSec; 
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



void UPJTTSClientMinimaxWebSocket::OnTTSResponse(EPJSpeechResponseState ResponseState)
{
	TArray<uint8> AudioBuffer;
	if (ResponseState == EPJSpeechResponseState::PartialResult)
	{
		AudioBuffer = TTSSynthesizePartialBuffer;
		TTSSynthesizeFinalAudioBuffer.Append(AudioBuffer);
		TTSSynthesizePartialBuffer.Empty();
		UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("(RequestID:%d) PartialResult "), BackupRequestParams.RequestID);
	}
	else if (ResponseState == EPJSpeechResponseState::Finished)
	{
		AudioBuffer = TTSSynthesizeFinalAudioBuffer;
		TTSSynthesizeFinalAudioBuffer.Empty();
		UE_LOG(PJTTSClientMinimaxWebSocket, Log, TEXT("(RequestID:%d) Finished "), BackupRequestParams.RequestID);
	}

	if (BackupRequestParams.ResponseLambda.IsSet())
	{
		BackupRequestParams.ResponseLambda(BackupRequestParams.RequestID, ResponseState, AudioBuffer);
	}

	if (ResponseState != EPJSpeechResponseState::PartialResult)
	{
		ResetTTSInfo();
	}
}

void UPJTTSClientMinimaxWebSocket::ResetTTSInfo()
{
	BackupRequestParams.Reset();
	TTSSynthesizeFinalAudioBuffer.Empty();
	TTSSynthesizePartialBuffer.Empty();
}
