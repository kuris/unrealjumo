#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeatherControlWidget.generated.h"

class ASimpleWeatherActor;
class UTextBlock;
class USlider;
class ACharacter;

/** Touch control anchored to the top safe area, independent of the map's HUD. */
UCLASS()
class SIMPLEWEATHER_API UWeatherControlWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetWeatherActor(ASimpleWeatherActor* Actor);
    void RefreshLabel();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void HandleWeatherClicked();

    UFUNCTION()
    void HandleRainAmountChanged(float Value);

    UFUNCTION()
    void HandleRunPressed();

    UFUNCTION()
    void HandleJumpPressed();

    UFUNCTION()
    void HandleJumpReleased();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RunLabel;

    TWeakObjectPtr<ACharacter> RunningCharacter;
    TWeakObjectPtr<ACharacter> JumpingCharacter;
    float WalkingSpeed = 0.f;
    bool bRunning = false;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Label;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AmountLabel;

    UPROPERTY(Transient)
    TObjectPtr<USlider> RainSlider;

    bool bUpdatingControls = false;

    TWeakObjectPtr<ASimpleWeatherActor> WeatherActor;
};
