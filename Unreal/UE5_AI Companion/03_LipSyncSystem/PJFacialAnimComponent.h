// AI Character Companion Application
// Built with Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PJFacialAnimComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPJFacialAnim, Log, All);

/**
 * 멀티플랫폼 실시간 페이셜 애니메이션 컴포넌트
 *
 * [아키텍처]
 * - Traditional 모드:
 *   - iOS: CRI 미들웨어 (AtomAudioLink) 기반 5모음 비짐 추출
 *   - 기타 플랫폼: RMH (RuntimeVisemeGenerator) 기반 15종 비짐 추출
 *   - 플랫폼 분기를 #if PLATFORM_IOS 전처리기로 처리
 *
 * - Audio2Face 모드:
 *   - NVIDIA A2F gRPC 스트리밍으로 ARKit 52 블렌드셰이프 수신
 *   - 립싱크 + 페이셜 익스프레션 동시 구동
 *   - CRI/RMH 비짐 분석 비활성화
 *
 * [데이터 흐름 - Traditional]
 * 1. PCM 오디오 버퍼 수신 (TTS 시스템 또는 파일)
 * 2. ProcessVisemeInput: PCM -> 비짐 분석 엔진에 프레임 단위 주입
 * 3. ProcessAudioInput: PCM -> StreamingSoundWave로 오디오 출력
 * 4. ExtractFacialResults: 비짐 가중치 추출 -> 캐릭터 모프 타겟에 적용
 *
 * [데이터 흐름 - Audio2Face]
 * 1. PCM 오디오 버퍼 수신 (TTS 시스템)
 * 2. PCM -> A2F gRPC 스트리밍 전송
 * 3. ProcessAudioInput: PCM -> StreamingSoundWave로 오디오 출력
 * 4. A2F 응답 콜백 -> 52 블렌드셰이프 가중치 -> SkeletalMesh 모프 타겟 직접 적용
 *
 * [지원 입력]
 * - PCM 버퍼 (실시간 TTS 스트리밍)
 * - WAV 파일 (RuntimeAudioImporter)
 */

class USoundWaveProcedural;
class UCapturableSoundWave;
class UImportedSoundWave;
class UStreamingSoundWave;
class UAudioComponent;
class URuntimeAudioImporterLibrary;
class APJCostumeCharacter;
class UPJAudio2FaceClient;
struct FA2FBlendShapeFrame;

enum class EPJFacialPlayType : uint8
{
	None,
	PCMBuffer,
	Wave,
};

// 페이셜 분석 모드
enum class EPJFacialMode : uint8
{
	Traditional,	// CRI(iOS) / RMH(기타) 비짐 기반
	Audio2Face,		// NVIDIA A2F 블렌드셰이프 기반 (립싱크 + 페이셜)
};

DECLARE_MULTICAST_DELEGATE(FPJOnFacialAnimStarted);
DECLARE_MULTICAST_DELEGATE(FPJOnFacialAnimEnded);

UCLASS()
class PJCORE_API UPJFacialAnimComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	FPJOnFacialAnimStarted OnFacialAnimStarted;
	FPJOnFacialAnimEnded OnFacialAnimEnded;

	UPJFacialAnimComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetFacialMode(EPJFacialMode InMode);
	EPJFacialMode GetFacialMode() const { return FacialMode; }

	UFUNCTION(BlueprintCallable)
	void PlayFromFile(USoundBase* InSound);
	void PlayFromImportedSoundWave(UImportedSoundWave* InSoundWave);
	void PlayFromBuffer(const TArray<float>& InPCMFloat, const TArray<int16>& InPCMInt16, int32 InSampleRate, int32 InNumChannels);
	void Stop();

	const TArray<FString>& GetTargetNames() const { return FacialTargetNames; }
	const TArray<float>& GetTargetValues() const { return FacialTargetValues; }

protected:
	UFUNCTION()
	void OnAudioImportResult(class URuntimeAudioImporterLibrary* Importer, UImportedSoundWave* InSoundWave, ERuntimeImportStatus Status);

	void OnPlaybackFinished();

private:
	void ProcessVisemeInput(float DeltaTime);
	void ProcessAudioInput();
	void ExtractFacialResults();
	void StartPlayback();

	bool IsPlaying() const;

//------------- 플랫폼별 Facial (UPROPERTY로 GC 참조 보장) -----
private:
	UPROPERTY()
	TObjectPtr<class UAtomAudioLinkComponent> CRI_AtomAudioLinkComponent;
	TArray<int32> CRI_FacialTargetIndex;

	UPROPERTY()
	TObjectPtr<class URuntimeVisemeGenerator> RMH_VisemeGenerator;

//------------- 공용 변수 -------------------------------------
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FacialAnim")
	TObjectPtr<UAudioComponent> AudioComponent;

	// === Audio2Face ===
	void OnA2FBlendShapeFrameReceived(const FA2FBlendShapeFrame& Frame);
	void ApplyA2FBlendShapesToMesh(const FA2FBlendShapeFrame& Frame);

	UPROPERTY()
	TObjectPtr<UPJAudio2FaceClient> A2FClient;

	EPJFacialMode FacialMode = EPJFacialMode::Traditional;

private:
	EPJFacialPlayType FacialPlayType = EPJFacialPlayType::None;

	UPROPERTY(Transient)
	TObjectPtr<URuntimeAudioImporterLibrary> AudioImporterLib;
	UPROPERTY(Transient)
	TObjectPtr<UStreamingSoundWave> StreamingSoundWave;

	int32 SampleRate = 0;
	int32 NumChannels = 0;

	// 비짐/블렌드셰이프 타겟 이름 (Traditional: 15종, A2F: 52종)
	UPROPERTY(EditDefaultsOnly, Category = "FacialAnim")
	TArray<FString> FacialTargetNames;

	TArray<float>	FacialTargetValues;
	TArray<float>	PCMFloatBuffer;
	TArray<int16>	PCMInt16Buffer;

private:
	TWeakObjectPtr<APJCostumeCharacter> OwnerCharacter;
};
