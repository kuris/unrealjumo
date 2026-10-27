#include "WeatherControlWidget.h"
#include "SimpleWeatherActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UWeatherControlWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    USafeZone* SafeArea = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("WeatherSafeArea"));
    SafeArea->SetSidesToPad(true, true, true, false);
    SafeArea->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = SafeArea;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WeatherCanvas"));
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    SafeArea->SetContent(Canvas);

    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WeatherToggle"));
    Button->SetBackgroundColor(FLinearColor(0.025f, 0.08f, 0.14f, 0.98f));
    Button->SetClickMethod(EButtonClickMethod::DownAndUp);
    // Handle the initial press so a mobile viewport cannot swallow touch-up.
    Button->SetTouchMethod(EButtonTouchMethod::Down);
    Button->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleWeatherClicked);
    UCanvasPanelSlot* Position = Canvas->AddChildToCanvas(Button);
    Position->SetAnchors(FAnchors(0.5f, 0.f));
    Position->SetAlignment(FVector2D(0.5f, 0.f));
    Position->SetPosition(FVector2D(0.f, 8.f));
    Position->SetSize(FVector2D(280.f, 60.f));

    Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeatherLabel"));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = 20;
    Label->SetFont(Font);
    Label->SetJustification(ETextJustify::Center);
    Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    Button->SetContent(Label);

    AmountLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RainAmountLabel"));
    AmountLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Font.Size = 18;
    AmountLabel->SetFont(Font);
    AmountLabel->SetJustification(ETextJustify::Center);
    AmountLabel->SetShadowColorAndOpacity(FLinearColor::Black);
    AmountLabel->SetShadowOffset(FVector2D(1.f, 1.f));
    AmountLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* AmountPosition = Canvas->AddChildToCanvas(AmountLabel);
    AmountPosition->SetAnchors(FAnchors(0.5f, 0.f));
    AmountPosition->SetAlignment(FVector2D(0.5f, 0.f));
    AmountPosition->SetPosition(FVector2D(0.f, 76.f));
    AmountPosition->SetSize(FVector2D(280.f, 28.f));

    RainSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("RainAmount"));
    RainSlider->SetStepSize(0.01f);
    RainSlider->SetSliderBarColor(FLinearColor(0.1f, 0.35f, 0.5f, 1.f));
    RainSlider->SetSliderHandleColor(FLinearColor::White);
    RainSlider->OnValueChanged.AddDynamic(this, &UWeatherControlWidget::HandleRainAmountChanged);
    UCanvasPanelSlot* SliderPosition = Canvas->AddChildToCanvas(RainSlider);
    SliderPosition->SetAnchors(FAnchors(0.5f, 0.f));
    SliderPosition->SetAlignment(FVector2D(0.5f, 0.f));
    SliderPosition->SetPosition(FVector2D(0.f, 104.f));
    SliderPosition->SetSize(FVector2D(280.f, 52.f));

    // Keep action controls clear of the bottom movement/camera joysticks.
    auto AddActionButton = [&](const TCHAR* Name, const FText& Text, float Y, UTextBlock*& OutLabel)
    {
        UButton* Action = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        Action->SetBackgroundColor(FLinearColor(0.025f, 0.08f, 0.14f, 0.85f));
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Action);
        Slot->SetAnchors(FAnchors(1.f, 0.f));
        Slot->SetAlignment(FVector2D(1.f, 0.f));
        Slot->SetPosition(FVector2D(-24.f, Y));
        Slot->SetSize(FVector2D(130.f, 64.f));
        OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        OutLabel->SetFont(Font);
        OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        OutLabel->SetJustification(ETextJustify::Center);
        OutLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
        OutLabel->SetText(Text);
        Action->SetContent(OutLabel);
        return Action;
    };
    UTextBlock* RunText = nullptr;
    UButton* RunButton = AddActionButton(TEXT("CharacterRun"),
        NSLOCTEXT("SimpleWeather", "Run", "달리기 OFF"), 180.f, RunText);
    RunLabel = RunText;
    RunButton->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleRunPressed);
    UTextBlock* JumpText = nullptr;
    UButton* JumpButton = AddActionButton(TEXT("CharacterJump"),
        NSLOCTEXT("SimpleWeather", "Jump", "점프"), 256.f, JumpText);
    JumpButton->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleJumpPressed);
    JumpButton->OnReleased.AddDynamic(this, &UWeatherControlWidget::HandleJumpReleased);
    RefreshLabel();
}

void UWeatherControlWidget::HandleRunPressed()
{
    ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Character || !Character->GetCharacterMovement()) return;
    if (RunningCharacter.Get() != Character)
    {
        if (RunningCharacter.IsValid() && bRunning)
            RunningCharacter->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
        RunningCharacter = Character;
        WalkingSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
        bRunning = false;
    }
    bRunning = !bRunning;
    Character->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed * (bRunning ? 1.6f : 1.f);
    RunLabel->SetText(bRunning ? NSLOCTEXT("SimpleWeather", "RunOn", "달리기 ON")
        : NSLOCTEXT("SimpleWeather", "Run", "달리기 OFF"));
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_INPUT: run %s, speed %.0f cm/s"),
        bRunning ? TEXT("ON") : TEXT("OFF"), Character->GetCharacterMovement()->MaxWalkSpeed);
}

void UWeatherControlWidget::HandleJumpPressed()
{
    if (ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn()))
    {
        JumpingCharacter = Character;
        Character->Jump();
        UE_LOG(LogTemp, Display, TEXT("CHARACTER_INPUT: jump pressed, can jump %d"), Character->CanJump());
    }
}

void UWeatherControlWidget::HandleJumpReleased()
{
    if (JumpingCharacter.IsValid()) JumpingCharacter->StopJumping();
    JumpingCharacter.Reset();
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_INPUT: jump released"));
}

void UWeatherControlWidget::NativeDestruct()
{
    HandleJumpReleased();
    if (RunningCharacter.IsValid() && bRunning)
        RunningCharacter->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
    RunningCharacter.Reset();
    bRunning = false;
    Super::NativeDestruct();
}

void UWeatherControlWidget::SetWeatherActor(ASimpleWeatherActor* Actor)
{
    WeatherActor = Actor;
    RefreshLabel();
}

void UWeatherControlWidget::RefreshLabel()
{
    if (!Label) return;
    const bool bRain = WeatherActor.IsValid() && WeatherActor->GetWeather() == ESimpleWeather::Rain;
    Label->SetText(bRain
        ? NSLOCTEXT("SimpleWeather", "RainButton", "날씨 전환: 비 → 맑음")
        : NSLOCTEXT("SimpleWeather", "ClearButton", "날씨 전환: 맑음 → 비"));
    if (AmountLabel && RainSlider)
    {
        const TGuardValue<bool> Updating(bUpdatingControls, true);
        const float Amount = WeatherActor.IsValid() ? WeatherActor->GetRainAmount() : 1.f;
        RainSlider->SetValue(Amount);
        AmountLabel->SetText(FText::Format(NSLOCTEXT("SimpleWeather", "RainAmount", "비의 양: {0}%"),
            FText::AsNumber(FMath::RoundToInt(Amount * 100.f))));
    }
}

void UWeatherControlWidget::HandleRainAmountChanged(float Value)
{
    if (!bUpdatingControls && WeatherActor.IsValid()) WeatherActor->SetRainAmount(Value);
}

void UWeatherControlWidget::HandleWeatherClicked()
{
    UE_LOG(LogTemp, Display, TEXT("WEATHER_INPUT: top weather button pressed"));
    if (WeatherActor.IsValid()) WeatherActor->ToggleWeather();
}
