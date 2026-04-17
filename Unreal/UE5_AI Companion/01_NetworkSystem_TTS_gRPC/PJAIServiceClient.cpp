// AI Character Companion Application
// Built with Unreal Engine 5
#include "PJAIServiceClient.h"
#include "ControlFlowManager.h"
#include "ControlFlow.h"


DEFINE_LOG_CATEGORY(LogPJAIServiceClient);

static EGrpcPJAgentServerACCOUNT_TYPE ToServerAccountType(EPJAccountType type)
{
	switch (type)
	{
	case EPJAccountType::GOOGLE:
		return EGrpcPJAgentServerACCOUNT_TYPE::ACCOUNT_TYPE_Google;
	case EPJAccountType::APPLE:
		return EGrpcPJAgentServerACCOUNT_TYPE::ACCOUNT_TYPE_Apple;
	default:
		return EGrpcPJAgentServerACCOUNT_TYPE::ACCOUNT_TYPE_None;
	}
}

UPJAIServiceClient* UPJAIServiceClient::Get()
{
	if (UPJNetworkManager::Get())
	{
		return UPJNetworkManager::Get()->GetAIServiceClient();
	}

	UE_LOG(LogPJAIServiceClient, Error, TEXT("UPJAIServiceClient null"));

	return nullptr;
}

void UPJAIServiceClient::Init(const FString& EndPoint)
{
	AccountService = CreateGrpcService<UAiContent_AccountService>(EndPoint);
	ContentService = CreateGrpcService<UAiContent_ContentService>(EndPoint);
}

void UPJAIServiceClient::CreateAIChatService(const FString& AIChatEndPoint)
{
	AiChatService = CreateGrpcService<UAiChat_ContentService>(AIChatEndPoint);
}


// ═══════════════════════════════════════════════════════════════════════
//  채팅
// ═══════════════════════════════════════════════════════════════════════

void UPJAIServiceClient::SendChatMessage(const FString& CharacterGuid, const FString& Message, const FString& Prompt, UAiChat_ContentService::FAiChatResponseLambda ResponseLambda)
{
	if (!AiChatService)
	{
		return;
	}

	FGrpcPJAgentServerREQ_AI_CHAT ChatRequest;
	ChatRequest.ChatMessage = Message;
	ChatRequest.Prompt = Prompt;
	ChatRequest.ChatType = EGrpcPJAgentServerAI_CHAT_TYPE::AI_CHAT_TYPE_TEXT;
	ChatRequest.CharacterGuid = CharacterGuid;

	AiChatService->CallAiChat(ChatRequest, ResponseLambda, RequestMetaData);
}


// ═══════════════════════════════════════════════════════════════════════
//  로그인 플로우 (ControlFlow 기반 멀티스텝)
// ═══════════════════════════════════════════════════════════════════════

TSharedPtr<FControlFlow> UPJAIServiceClient::CreateLoginFlow(const FString& UserId, const FString& UserPassword, const FString& Token, EPJAccountType AccountType)
{
	RequestMetaData.MetaData.Empty();

	FControlFlow& LoginFlow = FControlFlowStatics::Create(this, TEXT("LoginFlow"));

	LoginFlow.QueueStep(TEXT("RequestLogin"), this, &ThisClass::LoginStep_RequestLogin, UserId, UserPassword, Token, AccountType);
	LoginFlow.QueueStep(TEXT("RequestAIChatServerInfo"), this, &ThisClass::LoginStep_RequestAIChatServerInfo);
	LoginFlow.QueueStep(TEXT("RequestAIChatAuthKey"), this, &ThisClass::LoginStep_RequestAIChatAuthKey);

	return LoginFlow.AsShared();
}

void UPJAIServiceClient::LoginStep_RequestLogin(FControlFlowNodeRef SubFlow, FString UserId, FString UserPassword, FString PlatformSignToken, EPJAccountType AccountType)
{
	FGrpcPJAgentServerREQ_LOGIN Request;
	Request.Id = UserId;
	Request.Pwd = UserPassword;
	Request.LoginToken = PlatformSignToken;
	Request.AccountType = ToServerAccountType(AccountType);

	auto Response =
		[WeakThis = MakeWeakObjectPtr(this), SubFlow, AccountType](const FGrpcResult& GrpcResult, const FGrpcPJAgentServerACK_LOGIN& Response)
		{
			if (!WeakThis.IsValid())
			{
				SubFlow->CancelFlow();
				return;
			}
			else if (!GrpcResponse::IsSucceeded(GrpcResult, Response))
			{
				WeakThis->RaiseLoginFail(SubFlow->GetNodeName(), GrpcResult, Response.Result);
				SubFlow->CancelFlow();
				return;
			}

			WeakThis->RequestMetaData.MetaData.Add(TEXT("authorization"), Response.AuthKey);

			SubFlow->ContinueFlow();
		};

	AccountService->CallLogin(Request, Response);
}

void UPJAIServiceClient::LoginStep_RequestAIChatServerInfo(FControlFlowNodeRef SubFlow)
{
	FGrpcPJAgentServerREQ_AI_CHAT_SERVER_INFO Request = {};

	auto Response =
		[WeakThis = MakeWeakObjectPtr(this), SubFlow](const FGrpcResult& GrpcResult, const FGrpcPJAgentServerACK_AI_CHAT_SERVER_INFO& Response)
		{
			if (!WeakThis.IsValid())
			{
				SubFlow->CancelFlow();
				return;
			}
			else if (!GrpcResponse::IsSucceeded(GrpcResult, Response))
			{
				WeakThis->RaiseLoginFail(SubFlow->GetNodeName(), GrpcResult, Response.Result);
				SubFlow->CancelFlow();
				return;
			}

			WeakThis->CreateAIChatService(Response.ServerAddress);

			SubFlow->ContinueFlow();
		};

	ContentService->CallAiChatServerInfo(Request, Response, RequestMetaData);
}

void UPJAIServiceClient::LoginStep_RequestAIChatAuthKey(FControlFlowNodeRef SubFlow)
{
	FGrpcPJAgentServerREQ_AI_CHAT_AUTHKEY Request = {};

	auto Response =
		[WeakThis = MakeWeakObjectPtr(this), SubFlow](const FGrpcResult& GrpcResult, const FGrpcPJAgentServerACK_AI_CHAT_AUTHKEY& Response)
		{
			if (!WeakThis.IsValid())
			{
				SubFlow->CancelFlow();
				return;
			}
			else if (!GrpcResponse::IsSucceeded(GrpcResult, Response))
			{
				WeakThis->RaiseLoginFail(SubFlow->GetNodeName(), GrpcResult, Response.Result);
				SubFlow->CancelFlow();
				return;
			}

			WeakThis->AiAuthKeyChangedDelegate.Broadcast(Response.AuthKeys);

			SubFlow->ContinueFlow();
		};

	AiChatService->CallGetAiChatAuthKey(Request, Response, RequestMetaData);
}


// ═══════════════════════════════════════════════════════════════════════
//  에러 처리
// ═══════════════════════════════════════════════════════════════════════

void UPJAIServiceClient::RaiseLoginFail(const FString& Step, const FGrpcResult& GrpcResult, const FGrpcPJAgentServerRESULT& ServerResult)
{
	FPJLoginFailResult FailResult;
	FailResult.Step = Step;

	if (GrpcResult.Code != EGrpcResultCode::Ok)
	{
		FailResult.Category = TEXT("GRPC");
		FailResult.ResponseCode = (int32)GrpcResult.Code;
		FailResult.ErrorMessage = GrpcResult.Message;
	}
	else
	{
		FailResult.Category = TEXT("SERVER");
		FailResult.ResponseCode = (int32)ServerResult.Code;
		FailResult.ErrorMessage = ServerResult.Message;
	}

	LoginFailDelegate.Broadcast(FailResult);
}
