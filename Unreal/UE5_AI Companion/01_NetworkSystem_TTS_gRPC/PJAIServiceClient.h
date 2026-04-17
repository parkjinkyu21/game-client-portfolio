// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJAIServiceClient.generated.h"


/**
 * AI 서비스 클라이언트 (gRPC 기반)
 *
 * [핵심 기능]
 * - gRPC 기반 AI 서버와의 양방향 통신
 * - 멀티스텝 인증 플로우: Login -> ServerInfo -> AuthKey (ControlFlow 노드 기반)
 * - 템플릿 기반 gRPC 서비스 생성: CreateGrpcService<T>()
 *
 * [인증 플로우]
 * CreateLoginFlow() -> LoginStep_RequestLogin
 *                   -> LoginStep_RequestAIChatServerInfo
 *                   -> LoginStep_RequestAIChatAuthKey
 *
 * [메시지 흐름]
 * Client -> SendChatMessage() -> gRPC Request -> Response Lambda
 */

DECLARE_LOG_CATEGORY_EXTERN(LogPJAIServiceClient, Log, All);

UENUM()
enum class EPJAccountType : uint8
{
	ID_ACCOUNT,
	GOOGLE,
	APPLE,
	GUEST,
	MaxValue
};

struct FPJLoginFailResult
{
	FString	Step;
	FString	Category;
	int32	ResponseCode;
	FString	ErrorMessage;
};

namespace GrpcResponse
{
	template<typename TResponseMessageType>
	bool IsSucceeded(const FGrpcResult& Result, const TResponseMessageType& Response)
		requires TIsDerivedFrom<TResponseMessageType, FGrpcMessage>::IsDerived
	{
		return (Result.Code == EGrpcResultCode::Ok && Response.Result.Code == EGrpcPJAgentServerERR_CODE::ERRCODE_SUCCESS);
	}
}

UCLASS()
class PJNETWORK_API UPJAIServiceClient : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginFailDelegate, const FPJLoginFailResult&);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAiChatAuthKeyChanged, const TArray<FGrpcPJAgentServerAUTHKEY>&);

public:
	static UPJAIServiceClient* Get();

	void Init(const FString& EndPoint);

	TSharedPtr<FControlFlow> CreateLoginFlow(const FString& UserId, const FString& UserPassword, const FString& Token, EPJAccountType AccountType);

	void SendChatMessage(const FString& CharacterGuid, const FString& Message, const FString& Prompt, UAiChat_ContentService::FAiChatResponseLambda ResponseLambda);

	FOnLoginFailDelegate& OnLoginFailed() { return LoginFailDelegate; }
	FOnAiChatAuthKeyChanged& OnAiAuthKeyChanged() { return AiAuthKeyChangedDelegate; }

private:
	// === 로그인 스텝 ===
	void LoginStep_RequestLogin(FControlFlowNodeRef SubFlow, FString UserId, FString UserPassword, FString PlatformSignToken, EPJAccountType AccountType);
	void LoginStep_RequestAIChatServerInfo(FControlFlowNodeRef SubFlow);
	void LoginStep_RequestAIChatAuthKey(FControlFlowNodeRef SubFlow);

	void CreateAIChatService(const FString& AIChatEndPoint);

	void RaiseLoginFail(const FString& Step, const FGrpcResult& GrpcResult, const FGrpcPJAgentServerRESULT& ServerResult);

	template <typename TGrpcServiceType>
	TGrpcServiceType* CreateGrpcService(const FString& ServerEndPoint)
		requires TIsDerivedFrom<TGrpcServiceType, UGrpcService>::IsDerived
	{
		if (UGrpcServiceManager* GrpcMgr = UGrpcUtilities::GetGrpcServiceManager(this))
		{
			FString ServiceName = TGrpcServiceType::StaticClass()->GetName();

			GrpcMgr->SetServiceEndPoint(ServiceName, ServerEndPoint);

			if (UGrpcService* ServiceInstance = GrpcMgr->MakeService(ServiceName))
			{
				ServiceInstance->Connect();

				return Cast<TGrpcServiceType>(ServiceInstance);
			}
		}

		return nullptr;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<class UAiContent_AccountService>	AccountService;

	UPROPERTY(Transient)
	TObjectPtr<class UAiContent_ContentService>	ContentService;

	UPROPERTY(Transient)
	TObjectPtr<class UAiChat_ContentService>	AiChatService;

	FGrpcMetaData								RequestMetaData;

	FOnLoginFailDelegate						LoginFailDelegate;
	FOnAiChatAuthKeyChanged						AiAuthKeyChangedDelegate;
};
