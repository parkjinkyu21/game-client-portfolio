// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PJNetworkManager.generated.h"

class UPJAIServiceClient;
class UPJSpeechClient;
struct FGrpcMessage;
struct FGrpcResult;
struct FPJTTSRequestParams;
struct FPJSTTRequestParams;

/**
 * 네트워크 매니저 (GameInstanceSubsystem)
 *
 * 앱 전체의 네트워크 통신을 관장하는 싱글턴 서브시스템.
 * - AI 서버 통신 (gRPC): 인증 + 채팅
 * - TTS/STT 음성 서비스 라우팅
 * - gRPC 요청/응답 로깅을 위한 글로벌 이벤트 바인딩
 */

DECLARE_LOG_CATEGORY_EXTERN(PJNetworkManager, Log, All);

UCLASS()
class PJNETWORK_API UPJNetworkManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPJNetworkManager* Get();
	
	//override function from UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void InitEndPoint(const FString& EndPoint);

	UPJAIServiceClient* GetAIServiceClient() { return AIServiceClient.Get(); }
	
	void RequestTTS(const FPJTTSRequestParams& RequestParams);
	void RequestSTT(const FPJSTTRequestParams& RequestParams);

	void SendChatMessage(const FString& CharacterGuid, const FString& Message, const FString& Prompt, UAiChat_ContentService::FAiChatResponseLambda ResponseLambda);

private:
	void OnAiAuthKeyChanged(const TArray<FGrpcPJAgentServerAUTHKEY>& AuthKeys);

	bool bIsInitialized = false;

	UPROPERTY(Transient)
	TObjectPtr<UPJAIServiceClient> AIServiceClient;

	UPROPERTY(Transient)
	TObjectPtr<UPJSpeechClient> SpeechClient;

	UFUNCTION()
	void OnGrpcCalled(const FName& MessageName, const FGrpcMessage& Request);

	UFUNCTION()
	void OnGrpcReceived(const FGrpcResult& GrpcResult, const FName& MessageName, const FGrpcMessage& Response);

	void LogGrpcMessage(const FString& Name, const FString& Body, bool bError);
};

