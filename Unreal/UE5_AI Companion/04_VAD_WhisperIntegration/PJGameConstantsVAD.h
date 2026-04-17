// AI Character Companion Application
// Built with Unreal Engine 5

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PJGameConstantsVAD.generated.h"

/**
 * VAD(Voice Activity Detection) 설정 구조체
 *
 * UPJGameConstant에 포함되어 DefaultGame.ini에서 설정 가능.
 * Whisper Silero VAD 프로세서의 동작 파라미터를 런타임에 조정할 수 있도록 UPROPERTY로 노출.
 */
USTRUCT(BlueprintType)
struct PJCORE_API FPJGameConstantVADSetting
{
	GENERATED_BODY()

	// PCM 수집 주기 - 이 시간 동안 PCM을 모아서 VAD 분석 1회 수행
	UPROPERTY(EditAnywhere, config, Category = "PJVADSetting")
	float VADCycleTime = 0.1f;

	// 발화 시작 판정 임계값 (Silero VAD 확률값 기준)
	UPROPERTY(EditAnywhere, config, Category = "PJVADSetting")
	float SpeechStartThreshold = 0.35f;

	// 침묵 판정 임계값 (이 값 미만이면 침묵으로 간주)
	UPROPERTY(EditAnywhere, config, Category = "PJVADSetting")
	float SpeechEndThreshold = 0.2f;

	// 발화 종료 판정을 위한 침묵 지속 시간
	UPROPERTY(EditAnywhere, config, Category = "PJVADSetting")
	float SilenceTimeoutSec = 1.f;
};
