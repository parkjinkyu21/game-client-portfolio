// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJSpeechClient.h"
#include "TTSClient/PJTTSClientBase.h"
#include "STTClient/PJSTTClientCloudGrpc.h"

DEFINE_LOG_CATEGORY(PJSpeechClient);


void UPJSpeechClient::Init()
{
	CloudAuthKey = TEXT("");
	GeminiAuthKey = TEXT("");
}

void UPJSpeechClient::Clear()
{
	if (IsValid(CurrentSTTClient))
	{
		CurrentSTTClient->Disconnect();
	}
	CurrentSTTClient = nullptr;

	if (IsValid(CurrentTTSClient))
	{
		CurrentTTSClient->Disconnect();
	}
	CurrentTTSClient = nullptr;
}

void UPJSpeechClient::OnServiceAuthKeyChanged(const TArray<FGrpcPJAgentServerAUTHKEY>& AuthKeys)
{
	for (auto Key : AuthKeys)
	{
		if (Key.AiAuthKeyType == EGrpcPJAgentServerAI_CHAT_AUTH_KEY_TYPE::AI_CHAT_AUTH_KEY_TYPE_GOOGLE_CLOUD)
		{
			CloudAuthKey = Key.AuthKey;
		}
		else if (Key.AiAuthKeyType == EGrpcPJAgentServerAI_CHAT_AUTH_KEY_TYPE::AI_CHAT_AUTH_KEY_TYPE_GEMINI_OAUTH)
		{
			GeminiAuthKey = Key.AuthKey;
		}
	}

	// STT 클라이언트 연결 (Google Cloud gRPC)
	CurrentSTTClient = NewObject<UPJSTTClientCloudGrpc>(this);
	CurrentSTTClient->Connect(CloudAuthKey);
}

FString UPJSpeechClient::GetTTSClientAuthToken(EPJTTSClientType ClientType)
{
	if(ClientType == EPJTTSClientType::CloudGrpc)
		return CloudAuthKey;
	else if (ClientType == EPJTTSClientType::MinimaxWebSocket)
		return TEXT("");

	return GeminiAuthKey;
}

void UPJSpeechClient::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	EPJTTSClientType ClientType = RequestParams.TTSClientType;

	if (!IsValid(CurrentTTSClient) || CurrentTTSClient->GetClientType() != ClientType)
	{
		CurrentTTSClient = nullptr;
		CurrentTTSClient = UPJTTSClientBase::CreateClient(ClientType);
		CurrentTTSClient->Connect(GetTTSClientAuthToken(ClientType));
	}

	CurrentTTSClient->RequestTTS(RequestParams);
}

void UPJSpeechClient::RequestSTT(const FPJSTTRequestParams& RequestParams)
{
	if (!IsValid(CurrentSTTClient))
	{
		return;
	}

	CurrentSTTClient->RequestSTT(RequestParams);
}
