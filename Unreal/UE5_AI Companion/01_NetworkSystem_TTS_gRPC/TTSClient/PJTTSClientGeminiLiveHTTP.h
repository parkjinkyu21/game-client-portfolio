// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "PJTTSClientBase.h"
#include "PJTTSClientGeminiLiveHTTP.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PJTTSClientGeminiLiveHTTP, Log, All);

class IHttpRequest;
class IHttpResponse;

typedef TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> FHttpRequestPtr;
typedef TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> FHttpResponsePtr;

UCLASS(Blueprintable)
class PJNETWORK_API UPJTTSClientGeminiLiveHTTP : public UPJTTSClientBase
{
    GENERATED_BODY()

public:
    virtual void RequestTTS(const FPJTTSRequestParams& RequestParams);

private:
	void OnResponse(FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bWasSuccessful);
	FString GetAudioDataFromGeminiTTSResponse(const FString& ResponseString);

};