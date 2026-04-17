// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJTTSClientBase.h"
#include "PJTTSClientGeminiWebSocket.h"
#include "PJTTSClientMinimaxWebSocket.h"
#include "PJTTSClientGeminiLiveHTTP.h"
#include "PJTTSClientCloudGrpc.h"


UPJTTSClientBase* UPJTTSClientBase::CreateClient(EPJTTSClientType ClientType)
{
	UPJTTSClientBase* TTSClient = nullptr;
	if (ClientType == EPJTTSClientType::GeminiWebSocket)
	{
		// 기본 설정은 기획자가 BP 클래스에서 제공하도록 한다.
		UClass* GeminiTTSClass = LoadClass<UPJTTSClientGeminiWebSocket>(nullptr, TEXT("/Game/CommonAssets/BluePrints/BP_GeminiTTSWebSocket.BP_GeminiTTSWebSocket_C"));
		if (GeminiTTSClass)
		{
			TTSClient = NewObject<UPJTTSClientGeminiWebSocket>(GetTransientPackage(), GeminiTTSClass);
		}
	}
	else if (ClientType == EPJTTSClientType::MinimaxWebSocket)
	{
		// 기본 설정은 기획자가 BP 클래스에서 제공하도록 한다.
		UClass* MinimaxTTSClass = LoadClass<UPJTTSClientMinimaxWebSocket>(nullptr, TEXT("/Game/CommonAssets/BluePrints/BP_MinimaxTTSWebSocket.BP_MinimaxTTSWebSocket_C"));
		if (MinimaxTTSClass)
		{
			TTSClient = NewObject<UPJTTSClientMinimaxWebSocket>(GetTransientPackage(), MinimaxTTSClass);
		}
	}
	else if (ClientType == EPJTTSClientType::GeminiLiveProHTTP || ClientType == EPJTTSClientType::GeminiLiveFlashHTTP )
	{
		TTSClient = NewObject<UPJTTSClientGeminiLiveHTTP>(GetTransientPackage());
	}
	else if (ClientType == EPJTTSClientType::CloudGrpc)
	{
		TTSClient = NewObject<UPJTTSClientCloudGrpc>(GetTransientPackage());
	}

	if (TTSClient)
	{
		TTSClient->SetClientType(ClientType);
	}

	ensure(TTSClient);

	return TTSClient;
}


UPJTTSClientBase::~UPJTTSClientBase()
{
	Disconnect();
}

void UPJTTSClientBase::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	FPJVoiceModelData* VoiceModelData = UPJDataTables::Resolve<UPJVoiceModelDataTable>(RequestParams.VoiceModelId);
	if (VoiceModelData)
	{
		VoiceLanguage = VoiceModelData->Lang;
		switch (ClientType)
		{
		case EPJTTSClientType::GeminiWebSocket:
			VoiceName = VoiceModelData->GeminiWebsocketVoiceName;
			break;
		case EPJTTSClientType::MinimaxWebSocket:
			VoiceName = VoiceModelData->MinimaxVoiceName;
			break;
		case EPJTTSClientType::GeminiLiveProHTTP:
		case EPJTTSClientType::GeminiLiveFlashHTTP:
			VoiceName = VoiceModelData->GeminiRestVoiceName;
			break;
		case EPJTTSClientType::CloudGrpc:
			VoiceName = VoiceModelData->CloudVoiceName;
			break;
		default:
			ensure(0);
			break;
		}
	}
}