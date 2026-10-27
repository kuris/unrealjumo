#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimpleWeatherActor.generated.h"

class UAudioComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UWeatherControlWidget;
class UPostProcessComponent;

UENUM(BlueprintType)
enum class ESimpleWeather : uint8
{
    Clear,
    Rain
};

/** Self-contained environment. Place only in maps that opt into this lighting rig. */
UCLASS(Blueprintable)
class SIMPLEWEATHER_API ASimpleWeatherActor : public AActor
{
    GENERATED_BODY()

public:
    ASimpleWeatherActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Weather")
    void SetWeather(ESimpleWeather NewWeather);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="Weather")
    void ToggleWeather();

    UFUNCTION(BlueprintPure, Category="Weather")
    ESimpleWeather GetWeather() const { return Weather; }

    UFUNCTION(BlueprintCallable, Category="Weather")
    void SetRainAmount(float NewAmount);

    UFUNCTION(BlueprintPure, Category="Weather")
    float GetRainAmount() const { return RainAmount; }

    UFUNCTION(BlueprintPure, Category="Weather")
    int32 GetActiveRainDropCount() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather", meta=(ClampMin="0", ClampMax="1"))
    float RainAmount = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    bool bStartClear = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    ESimpleWeather Weather = ESimpleWeather::Clear;

    /** Opt-in test control. Does not consume R from the character. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    bool bEnableTestKey = false;

    /** Top safe-area touch button for maps that include this weather actor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    bool bShowWeatherButton = true;

    /** Optional Fab Weather System Blueprint placed in this level. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Weather|Weather System")
    TObjectPtr<AActor> WeatherSystemActor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    TObjectPtr<UInstancedStaticMeshComponent> Rain;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    TObjectPtr<UDirectionalLightComponent> Sun;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    TObjectPtr<USkyAtmosphereComponent> Atmosphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    TObjectPtr<UExponentialHeightFogComponent> Fog;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather|Audio")
    TObjectPtr<UAudioComponent> WindAudio;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather|Audio")
    TObjectPtr<UAudioComponent> SurfAudio;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void BuildRain();
    void SetupPlayerLocomotion();
    FTimerHandle LocomotionSetupTimer;
    void UpdateRainDensity();
    void UpdateWeatherSystem();
    TArray<FTransform> Drops;

    UPROPERTY(Transient)
    TObjectPtr<UWeatherControlWidget> WeatherControls;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> RuntimeExposure;
};
