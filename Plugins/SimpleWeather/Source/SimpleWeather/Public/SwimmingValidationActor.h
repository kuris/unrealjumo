#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwimmingValidationActor.generated.h"
class ACharacter;
class ACameraActor;
class UFairySwimmingComponent;
class UJumoLocomotionComponent;

/** Opt-in integration test, spawned only with -SwimmingValidate. */
UCLASS()
class SIMPLEWEATHER_API ASwimmingValidationActor : public AActor
{
    GENERATED_BODY()
public:
    ASwimmingValidationActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void Check(bool bCondition, const FString& Description);
    void Shot(const FString& Name);
    void SaveCapture(int32 Width, int32 Height, const TArray<FColor>& Pixels);
    void Finish();
    UPROPERTY() TObjectPtr<ACharacter> Player;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<UFairySwimmingComponent> Swimming;
    UPROPERTY() TObjectPtr<UJumoLocomotionComponent> Playback;
    TWeakObjectPtr<AActor> OriginalView;
    FTransform OriginalTransform;
    FVector SwimStart;
    float Elapsed = 0.f;
    float StageTime = 0.f;
    float WalkingSpeed = 0.f;
    float OriginalBuoyancy = 0.f;
    float OriginalSwimSpeed = 0.f;
    float OriginalAutoEnterDepth = 100.f;
    bool bOriginalVolumeUpdates = true;
    bool bFailed = false;
    bool bFinished = false;
    int32 Phase = 0;
    FString Output;
    FString PendingCapture;
    FDelegateHandle CaptureHandle;
};
