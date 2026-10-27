#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeatherControlWidget.generated.h"

class ASimpleWeatherActor;
class UTextBlock;
class USlider;
class ACharacter;
class UCanvasPanel;
class UCanvasPanelSlot;

/** Touch control anchored to the top safe area, independent of the map's HUD. */
UCLASS()
class SIMPLEWEATHER_API UWeatherControlWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetWeatherActor(ASimpleWeatherActor* Actor);
    void RefreshLabel();
    UUserWidget* GetLegacyTouchWidget() const { return LegacyTouchWidget.Get(); }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

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

    UFUNCTION()
    void HandleSwimPressed();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RunLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SwimLabel;

    TWeakObjectPtr<ACharacter> RunningCharacter;
    TWeakObjectPtr<ACharacter> JumpingCharacter;
    float WalkingSpeed = 0.f;
    bool bRunning = false;
    bool bSwimControlsDisabled = false;
    bool bRunEnabledBeforeSwim = true;
    bool bJumpEnabledBeforeSwim = true;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Label;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AmountLabel;

    UPROPERTY(Transient)
    TObjectPtr<USlider> RainSlider;

    bool bUpdatingControls = false;

    TWeakObjectPtr<ASimpleWeatherActor> WeatherActor;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ControlCanvas;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> RunPosition;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> JumpPosition;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> SwimPosition;
    TWeakObjectPtr<UUserWidget> LegacyTouchWidget;
    TWeakObjectPtr<UWidget> LegacyJumpButton;
    ESlateVisibility LegacyJumpVisibility = ESlateVisibility::Visible;
    bool bLegacyJumpEnabled = true;
};
