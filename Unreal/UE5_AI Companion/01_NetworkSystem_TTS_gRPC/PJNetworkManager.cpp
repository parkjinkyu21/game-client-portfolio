// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJNetworkManager.h"

DEFINE_LOG_CATEGORY(PJNetworkManager);

UPJNetworkManager* UPJNetworkManager::Get()
{
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
		{
			if (UWorld* World = Context.World())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					return GI->GetSubsystem<UPJNetworkManager>();
				}
			}
		}
	}
	return nullptr;
}

void UPJNetworkManager::Initialize(FSubsystemCollectionBase& Collection)
{
	if (bIsInitialized) return;

	AIServiceClient = NewObject<UPJAIServiceClient>(this);
	AIServiceClient->OnAiAuthKeyChanged().AddUObject(this, &ThisClass::OnAiAuthKeyChanged);

	SpeechClient = NewObject<UPJSpeechClient>(this);
	
	if (UGrpcServiceManager* GrpcManager = UGrpcUtilities::GetGrpcServiceManager(nullptr))
	{
		GrpcManager->GlobalCallEvent.AddUObject(this, &ThisClass::OnGrpcCalled);
		GrpcManager->GlobalReceiveEvent.AddUObject(this, &ThisClass::OnGrpcReceived);
	}

	UE_LOG(PJNetworkManager, Log, TEXT("NetworkManager Initialized "));
}

void UPJNetworkManager::Deinitialize()
{
	UE_LOG(PJNetworkManager, Log, TEXT("NetworkManager Deinitialized "));
	SpeechClient->Clear();
	SpeechClient = nullptr;
	AIServiceClient = nullptr;
	bIsInitialized = false;
}

void UPJNetworkManager::InitEndPoint(const FString& EndPoint)
{
	AIServiceClient->Init(EndPoint);

	SpeechClient->Init();

	bIsInitialized = true;
}

void UPJNetworkManager::RequestTTS(const FPJTTSRequestParams& RequestParams)
{
	if (SpeechClient)
	{
		SpeechClient->RequestTTS(RequestParams);
	}
}
void UPJNetworkManager::RequestSTT(const FPJSTTRequestParams& RequestParams)
{
	if (SpeechClient)
	{
		SpeechClient->RequestSTT(RequestParams);
	}
}

void UPJNetworkManager::SendChatMessage(const FString& CharacterGuid, const FString& Message, const FString& Prompt, UAiChat_ContentService::FAiChatResponseLambda ResponseLambda)
{
	if (AIServiceClient)
	{
		AIServiceClient->SendChatMessage(CharacterGuid, Message, Prompt, ResponseLambda);
	}
}

void UPJNetworkManager::OnAiAuthKeyChanged(const TArray<FGrpcPJAgentServerAUTHKEY>& AuthKeys)
{
	if (SpeechClient)
	{
		SpeechClient->OnServiceAuthKeyChanged(AuthKeys);
	}
}

void UPJNetworkManager::OnGrpcCalled(const FName& MessageName, const FGrpcMessage& Request)
{
	FString Body = Request.ToJsonString(true);

	LogGrpcMessage(MessageName.ToString(), *Body, false);
}

void UPJNetworkManager::OnGrpcReceived(const FGrpcResult& GrpcResult, const FName& MessageName, const FGrpcMessage& Response)
{
	bool bHasError = (GrpcResult.Code != EGrpcResultCode::Ok);

	FString Body = Response.ToJsonString(true);

	LogGrpcMessage(MessageName.ToString(), *Body, bHasError);
}

void UPJNetworkManager::LogGrpcMessage(const FString& Name, const FString& Body, bool bError)
{
	if (bError)
	{
		UE_LOG(PJNetworkManager, Error, TEXT("[PACKET] %s %s"), *Name, *Body);
	}
	else
	{
		UE_LOG(PJNetworkManager, Log, TEXT("[PACKET] %s %s"), *Name, *Body);
	}
}