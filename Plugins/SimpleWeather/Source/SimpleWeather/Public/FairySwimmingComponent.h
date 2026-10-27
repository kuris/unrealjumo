#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FairySwimmingComponent.generated.h"

class ACharacter;
class APhysicsVolume;
class UWaterBodyComponent;

/** Surface-only swimming for the existing fairy; no changes to the pawn asset. */
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class SIMPLEWEATHER_API UFairySwimmingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFairySwimmingComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    UFUNCTION(BlueprintPure, Category="Swimming") bool IsSurfaceSwimming() const { return bSwimming; }
    UFUNCTION(BlueprintPure, Category="Swimming") float GetSurfaceHeight() const { return SurfaceHeight; }
    UFUNCTION(BlueprintPure, Category="Swimming") bool CanStartSwimming() const { return bCanStartSwimming; }
    UFUNCTION(BlueprintCallable, Category="Swimming") bool StartSwimming();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming", meta=(ClampMin="50")) float SwimSpeed = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming", meta=(ClampMin="20")) float EnterDepth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming", meta=(ClampMin="10")) float ExitDepth = 70.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming", meta=(ClampMin="20")) float ManualStartDepth = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming") float TreadSurfaceOffset = -55.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swimming") float ForwardSurfaceOffset = -25.f;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    bool QueryWater(float& OutSurface, float& OutDepth);
    void EnterWater();
    void LeaveWater(bool bRestoreMovement);
    void RefreshWaterBodies();
    TWeakObjectPtr<ACharacter> Character;
    TArray<TWeakObjectPtr<UWaterBodyComponent>> WaterBodies;
    UPROPERTY(Transient) TObjectPtr<APhysicsVolume> SwimmingVolume;
    TWeakObjectPtr<APhysicsVolume> PreviousVolume;
    bool bSwimming = false;
    bool bCanStartSwimming = false;
    bool bPreviousVolumeUpdates = true;
    float PreviousBuoyancy = 1.f;
    float PreviousSwimSpeed = 300.f;
    float SurfaceHeight = 0.f;
    float RefreshAt = 0.f;
};
