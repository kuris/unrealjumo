#include "WeatherControlWidget.h"
#include "FairySwimmingComponent.h"
#include "SimpleWeatherActor.h"
#include "IslandUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UObjectIterator.h"

void UWeatherControlWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    USafeZone* SafeArea = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("WeatherSafeArea"));
    SafeArea->SetSidesToPad(true, true, true, false);
    SafeArea->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = SafeArea;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WeatherCanvas"));
    ControlCanvas = Canvas;
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
    FSlateFontInfo Font = IslandUIStyle::Font(20);
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
        Action->SetTouchMethod(EButtonTouchMethod::Down);
        Action->SetClickMethod(EButtonClickMethod::MouseDown);
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
    RunPosition = Cast<UCanvasPanelSlot>(RunButton->Slot);
    RunPosition->SetAnchors(FAnchors(0.f, 0.f));
    RunPosition->SetAlignment(FVector2D::ZeroVector);
    RunButton->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleRunPressed);
    UTextBlock* JumpText = nullptr;
    UButton* JumpButton = AddActionButton(TEXT("CharacterJump"),
        NSLOCTEXT("SimpleWeather", "Jump", "점프"), 256.f, JumpText);
    JumpPosition = Cast<UCanvasPanelSlot>(JumpButton->Slot);
    JumpPosition->SetAnchors(FAnchors(0.f, 0.f));
    JumpPosition->SetAlignment(FVector2D::ZeroVector);
    // Jump retains touch capture so releasing the finger also releases Jump().
    JumpButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
    JumpButton->SetClickMethod(EButtonClickMethod::DownAndUp);
    JumpButton->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleJumpPressed);
    JumpButton->OnReleased.AddDynamic(this, &UWeatherControlWidget::HandleJumpReleased);
    UTextBlock* SwimText = nullptr;
    UButton* SwimButton = AddActionButton(TEXT("CharacterSwim"),
        NSLOCTEXT("SimpleWeather", "Swim", "수영하기"), 332.f, SwimText);
    SwimLabel = SwimText;
    SwimPosition = Cast<UCanvasPanelSlot>(SwimButton->Slot);
    SwimPosition->SetAnchors(FAnchors(0.f, 0.f));
    SwimPosition->SetAlignment(FVector2D::ZeroVector);
    SwimButton->SetVisibility(ESlateVisibility::Collapsed);
    SwimButton->OnPressed.AddDynamic(this, &UWeatherControlWidget::HandleSwimPressed);
    RefreshLabel();
}

void UWeatherControlWidget::HandleSwimPressed()
{
    if (!GetIsEnabled()) return;
    if (APawn* Pawn = GetOwningPlayerPawn())
        if (auto* Swimming = Pawn->FindComponentByClass<UFairySwimmingComponent>()) Swimming->StartSwimming();
}

void UWeatherControlWidget::HandleRunPressed()
{
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_INPUT: run touch received enabled=%d pawn=%s"), GetIsEnabled(), *GetNameSafe(GetOwningPlayerPawn()));
    if (!GetIsEnabled()) return;
    ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Character || !Character->GetCharacterMovement()) return;
    if (Character->GetCharacterMovement()->IsSwimming()) return;
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
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_INPUT: jump touch received enabled=%d pawn=%s"), GetIsEnabled(), *GetNameSafe(GetOwningPlayerPawn()));
    if (!GetIsEnabled()) return;
    if (ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn()))
    {
        if (Character->GetCharacterMovement()->IsSwimming()) return;
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
    if (LegacyJumpButton.IsValid())
    {
        LegacyJumpButton->SetVisibility(LegacyJumpVisibility);
        LegacyJumpButton->SetIsEnabled(bLegacyJumpEnabled);
    }
    HandleJumpReleased();
    if (RunningCharacter.IsValid() && bRunning)
        RunningCharacter->GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
    RunningCharacter.Reset();
    bRunning = false;
    Super::NativeDestruct();
}

void UWeatherControlWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    ACharacter* Pawn = Cast<ACharacter>(GetOwningPlayerPawn());
    const bool bSwimming = Pawn && Pawn->GetCharacterMovement()->IsSwimming();
    const auto* Swimming = Pawn ? Pawn->FindComponentByClass<UFairySwimmingComponent>() : nullptr;
    if (UWidget* SwimButton = GetWidgetFromName(TEXT("CharacterSwim")))
    {
        const bool bActive = Swimming && Swimming->IsSurfaceSwimming();
        const bool bCanStart = Swimming && Swimming->CanStartSwimming();
        SwimButton->SetVisibility(bActive || bCanStart ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        SwimButton->SetIsEnabled(bCanStart);
        if (SwimLabel) SwimLabel->SetText(bActive ? NSLOCTEXT("SimpleWeather", "Swimming", "수영 중")
            : NSLOCTEXT("SimpleWeather", "Swim", "수영하기"));
    }
    if (bSwimming != bSwimControlsDisabled)
    {
        UWidget* Run = GetWidgetFromName(TEXT("CharacterRun"));
        UWidget* Jump = GetWidgetFromName(TEXT("CharacterJump"));
        if (bSwimming)
        {
            bRunEnabledBeforeSwim = Run && Run->GetIsEnabled();
            bJumpEnabledBeforeSwim = Jump && Jump->GetIsEnabled();
            HandleJumpReleased();
        }
        if (Run) Run->SetIsEnabled(!bSwimming && bRunEnabledBeforeSwim);
        if (Jump) Jump->SetIsEnabled(!bSwimming && bJumpEnabledBeforeSwim);
        bSwimControlsDisabled = bSwimming;
    }
    if (!ControlCanvas || !RunPosition || !JumpPosition) return;
    if (!LegacyTouchWidget.IsValid())
    {
        for (TObjectIterator<UUserWidget> It; It; ++It)
        {
            if (It->GetWorld() == GetWorld() && It->IsInViewport() && *It != this
                && (!It->GetOwningPlayer() || It->GetOwningPlayer() == GetOwningPlayer())
                && It->GetWidgetFromName(TEXT("Thumbstick_Move")) && It->GetWidgetFromName(TEXT("Btn_Jump")))
            {
                LegacyTouchWidget = *It;
                LegacyJumpButton = It->GetWidgetFromName(TEXT("Btn_Jump"));
                LegacyJumpVisibility = LegacyJumpButton->GetVisibility();
                bLegacyJumpEnabled = LegacyJumpButton->GetIsEnabled();
                LegacyJumpButton->SetVisibility(ESlateVisibility::Collapsed);
                LegacyJumpButton->SetIsEnabled(false);
                UE_LOG(LogTemp, Display, TEXT("ISLAND_CONTROLS: replaced legacy Btn_Jump; actions beside movement stick"));
                break;
            }
        }
    }
    const FGeometry& CanvasGeometry = ControlCanvas->GetCachedGeometry();
    const FVector2D Size = CanvasGeometry.GetLocalSize();
    if (Size.X < 1.f || Size.Y < 1.f) return;
    FVector2D Beside(Size.X * 0.1f + FMath::Min(Size.X, Size.Y) * 0.16f + 16.f, Size.Y * 0.85f);
    float RightLimit = Size.X - 16.f;
    if (LegacyTouchWidget.IsValid())
    {
        if (UWidget* Stick = LegacyTouchWidget->GetWidgetFromName(TEXT("Thumbstick_Move")))
        {
            const FGeometry& StickGeometry = Stick->GetCachedGeometry();
            if (StickGeometry.GetLocalSize().X > 1.f)
                Beside = CanvasGeometry.AbsoluteToLocal(StickGeometry.LocalToAbsolute(FVector2D(StickGeometry.GetLocalSize().X, StickGeometry.GetLocalSize().Y * 0.5f))) + FVector2D(14.f, 0.f);
        }
        if (UWidget* Aim = LegacyTouchWidget->GetWidgetFromName(TEXT("Thumbstick_Aim")))
        {
            const FGeometry& AimGeometry = Aim->GetCachedGeometry();
            if (AimGeometry.GetLocalSize().X > 1.f)
                RightLimit = CanvasGeometry.AbsoluteToLocal(AimGeometry.LocalToAbsolute(FVector2D::ZeroVector)).X - 14.f;
        }
    }
    const float Width = FMath::Clamp(float(Size.X * 0.105f), 96.f, 130.f);
    const bool bStack = Beside.X + Width * 2.f + 12.f > RightLimit;
    const float X = FMath::Clamp(float(Beside.X), 12.f, FMath::Max(12.f, float(Size.X - Width - 12.f)));
    const float Y = FMath::Clamp(float(Beside.Y - (bStack ? 70.f : 32.f)), 12.f, FMath::Max(12.f, float(Size.Y - (bStack ? 140.f : 64.f) - 12.f)));
    RunPosition->SetSize(FVector2D(Width, 64.f)); JumpPosition->SetSize(FVector2D(Width, 64.f));
    RunPosition->SetPosition(FVector2D(X, Y));
    JumpPosition->SetPosition(FVector2D(X + (bStack ? 0.f : Width + 12.f), Y + (bStack ? 76.f : 0.f)));
    if (SwimPosition)
    {
        SwimPosition->SetSize(FVector2D(Width, 64.f));
        SwimPosition->SetPosition(FVector2D(X, FMath::Max(12.f, Y - 76.f)));
    }
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
    if (!GetIsEnabled()) return;
    if (!bUpdatingControls && WeatherActor.IsValid()) WeatherActor->SetRainAmount(Value);
}

void UWeatherControlWidget::HandleWeatherClicked()
{
    if (!GetIsEnabled()) return;
    UE_LOG(LogTemp, Display, TEXT("WEATHER_INPUT: top weather button pressed"));
    if (WeatherActor.IsValid()) WeatherActor->ToggleWeather();
}
