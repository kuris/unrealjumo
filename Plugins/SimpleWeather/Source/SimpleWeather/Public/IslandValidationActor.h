#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandValidationActor.generated.h"
class ACameraActor;
class ACharacter;
class AIslandPatrolCharacter;
class ASimpleWeatherActor;
class UWeatherControlWidget;
class UDialogueManagerComponent;

/** Opt-in packaged-game verification, enabled only by -IslandValidate. */
UCLASS()
class SIMPLEWEATHER_API AIslandValidationActor : public AActor
{
    GENERATED_BODY()
public:
    AIslandValidationActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void Check(bool bCondition, const FString& Description);
    void CheckWeatherSystem(float ExpectedAmount);
    void Shot(const FString& Name);
    void SaveCapture(int32 Width, int32 Height, const TArray<FColor>& Pixels);
    FDelegateHandle CaptureHandle;
    FString PendingCapture;
    void View(ACharacter* Character);
    void Press(const FName& Name, bool bRelease = false);
    void TickDialogueValidation();
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<ACharacter> Player;
    UPROPERTY() TObjectPtr<ASimpleWeatherActor> Weather;
    UPROPERTY() TObjectPtr<UWeatherControlWidget> Controls;
    UPROPERTY() TArray<TObjectPtr<AIslandPatrolCharacter>> Patrols;
    UPROPERTY() TObjectPtr<UDialogueManagerComponent> DialogueManager;
    UPROPERTY() TObjectPtr<AIslandPatrolCharacter> DialogueTestNpc;
    UPROPERTY() TObjectPtr<AActor> DialoguePreviousView;
    float DialogueStageTime = 0.f;
    int32 DialogueNpcIndex = 0;
    int32 DialogueStoppedPoints = 0;
    FVector DialoguePlayerHome = FVector::ZeroVector;
    FRotator DialogueControlHome = FRotator::ZeroRotator;
    FRotator DialogueExpectedControl = FRotator::ZeroRotator;
    FTransform DialoguePlayerBefore;
    FVector DialogueNpcBefore = FVector::ZeroVector;
    uint8 DialogueControlsVisibility = 0;
    bool bDialogueControlsEnabled = true;
    FName DialoguePendingButton;
    int32 DialoguePendingClickCount = 0;
    float DialoguePendingClickAt = 0.f;
    float Elapsed = 0.f;
    float BaseSpeed = 0.f;
    float JumpStartZ = 0.f;
    float MaxJumpRise = 0.f;
    int32 Phase = 0;
    int32 AttentionPatrolIndex = 0;
    int32 AttentionReachedPoints = 0;
    float AttentionPhaseStarted = 0.f;
    FVector AttentionStopLocation = FVector::ZeroVector;
    FVector PlayerReturnLocation = FVector::ZeroVector;
    bool bFailed = false;
    bool bValidationHoldingInput = false;
    FString Output;
};
