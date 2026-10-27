#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JumoLocomotionComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimSequence;

/** Opt-in playback for the weather test's original Jumo pawn. */
UCLASS()
class SIMPLEWEATHER_API UJumoLocomotionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UJumoLocomotionComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    FString GetLocomotionState() const { return CurrentState; }
    bool IsCompatible() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY() TObjectPtr<UAnimSequence> Idle;
    UPROPERTY() TObjectPtr<UAnimSequence> Walk;
    UPROPERTY() TObjectPtr<UAnimSequence> Run;
    UPROPERTY() TObjectPtr<UAnimSequence> Jump;
    UPROPERTY() TSubclassOf<UAnimInstance> OriginalAnimClass;
    TWeakObjectPtr<ACharacter> Character;
    float OriginalWalkingSpeed = 600.f;
    FString CurrentState;
};
