#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandValidationActor.generated.h"
class ACameraActor;
class ACharacter;
class AIslandPatrolCharacter;
class ASimpleWeatherActor;
class UWeatherControlWidget;

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
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<ACharacter> Player;
    UPROPERTY() TObjectPtr<ASimpleWeatherActor> Weather;
    UPROPERTY() TObjectPtr<UWeatherControlWidget> Controls;
    UPROPERTY() TArray<TObjectPtr<AIslandPatrolCharacter>> Patrols;
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
    FString Output;
};
