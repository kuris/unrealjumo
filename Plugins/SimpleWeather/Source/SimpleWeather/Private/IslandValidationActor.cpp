#include "IslandValidationActor.h"
#include "IslandPatrolCharacter.h"
#include "JumoLocomotionComponent.h"
#include "SimpleWeatherActor.h"
#include "WeatherControlWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/Button.h"
#include "Components/AudioComponent.h"
#include "Components/Slider.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"

AIslandValidationActor::AIslandValidationActor()
{
    PrimaryActorTick.bCanEverTick = true;
}
void AIslandValidationActor::Check(bool bCondition, const FString& Description)
{
    if (bCondition) { UE_LOG(LogTemp, Display, TEXT("ISLAND_CHECK_PASS: %s"), *Description); }
    else { bFailed = true; UE_LOG(LogTemp, Error, TEXT("ISLAND_CHECK_FAIL: %s"), *Description); }
}
void AIslandValidationActor::CheckWeatherSystem(float ExpectedAmount)
{
    if (!Weather || !Weather->WeatherSystemActor) return;
    UNiagaraComponent* Rain = Weather->WeatherSystemActor->FindComponentByClass<UNiagaraComponent>();
    Check(Rain != nullptr, TEXT("Fab Weather System Niagara component exists"));
    if (!Rain) return;
    bool bValid = false;
    const float Actual = Rain->GetVariableFloat(TEXT("User.SpawnRateMultiplyer"), bValid);
    Check(bValid && FMath::IsNearlyEqual(Actual, ExpectedAmount),
        FString::Printf(TEXT("Fab Niagara rainfall %.0f%% (actual %.2f)"), ExpectedAmount * 100.f, Actual));
    Check(Rain->IsVisible() == (ExpectedAmount > 0.f), TEXT("Fab rain visibility matches weather"));
    Check(!Weather->Rain->IsVisible() && Weather->Rain->GetInstanceCount() == 0,
        TEXT("Legacy cube rain disabled"));
    UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr,
        TEXT("/Game/WeatherSystem/Materials/MaterialParameterCollections/GeneralMPC.GeneralMPC"));
    float Humidity = -1.f;
    UMaterialParameterCollectionInstance* Instance = Collection ? GetWorld()->GetParameterCollectionInstance(Collection) : nullptr;
    Check(Instance && Instance->GetScalarParameterValue(TEXT("Humidity"), Humidity)
        && FMath::IsNearlyEqual(Humidity, ExpectedAmount), TEXT("Fab surface humidity matches weather"));
}
void AIslandValidationActor::Shot(const FString& Name)
{
    PendingCapture = Output / (Name + TEXT(".png"));
    FScreenshotRequest::RequestScreenshot(PendingCapture, false, false);
    UE_LOG(LogTemp, Display, TEXT("ISLAND_CAPTURE: %s"), *Name);
}
void AIslandValidationActor::View(ACharacter* Character)
{
    if (!Character || !Camera) return;
    const FVector Center = Character->GetActorLocation() + FVector(0, 0, 5);
    const FVector Eye = Center + FVector(430, 360, 80);
    Camera->SetActorLocation(Eye);
    Camera->SetActorRotation((Center - Eye).Rotation());
}
void AIslandValidationActor::Press(const FName& Name, bool bRelease)
{
    UButton* Button = Controls ? Cast<UButton>(Controls->GetWidgetFromName(Name)) : nullptr;
    Check(Button != nullptr, FString::Printf(TEXT("UI control exists: %s"), *Name.ToString()));
    if (Button) { if (bRelease) Button->OnReleased.Broadcast(); else Button->OnPressed.Broadcast(); }
}
void AIslandValidationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (Phase == 0 && Elapsed >= 3.f)
    {
        FParse::Value(FCommandLine::Get(), TEXT("IslandValidationOutput="), Output);
        if (Output.IsEmpty()) Output = FPaths::ProjectSavedDir() / TEXT("IslandValidation");
        Check(IFileManager::Get().MakeDirectory(*Output, true), TEXT("Capture directory writable: ") + Output);
        CaptureHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &AIslandValidationActor::SaveCapture);
        UE_LOG(LogTemp, Display, TEXT("ISLAND_CAPTURE_DIRECTORY: %s"), *IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Output));
        for (TActorIterator<AIslandPatrolCharacter> It(GetWorld()); It; ++It) Patrols.Add(*It);
        for (TActorIterator<ASimpleWeatherActor> It(GetWorld()); It; ++It) Weather = *It;
        for (TObjectIterator<UWeatherControlWidget> It; It; ++It)
            if (It->GetWorld() == GetWorld() && It->IsInViewport()) Controls = *It;
        Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        Check(Patrols.Num() == 2, TEXT("Exactly two hill patrols"));
        Check(PC && Player && Weather && Controls, TEXT("Player, weather and controls loaded"));
        if (!PC || !Player || !Weather || !Controls || Patrols.Num() != 2)
        { FPlatformMisc::RequestExitWithStatus(false, 1); return; }
        CheckWeatherSystem(0.f);
        Check(Weather->WindAudio && Weather->WindAudio->IsPlaying(), TEXT("Wind ambience playing"));
        Check(Weather->SurfAudio && Weather->SurfAudio->IsPlaying(), TEXT("Surf ambience playing"));
        BaseSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
        Check(Player->FindComponentByClass<UJumoLocomotionComponent>() != nullptr, TEXT("Jumo locomotion installed"));
        Camera = GetWorld()->SpawnActor<ACameraActor>();
        Camera->GetCameraComponent()->SetFieldOfView(50.f);
        PC->SetViewTarget(Camera);
        for (AIslandPatrolCharacter* Patrol : Patrols)
        {
            Check(Patrol->GetMesh()->GetUpVector().Z > 0.99f, Patrol->PatrolName + TEXT(" mesh stands upright"));
            Check(FMath::Abs(Patrol->GetActorRotation().Pitch) < 1.f && FMath::Abs(Patrol->GetActorRotation().Roll) < 1.f, Patrol->PatrolName + TEXT(" capsule stands upright"));
            Check(Patrol->PatrolPoints.Num() == 96, Patrol->PatrolName + TEXT(" has 96 route points"));
            Check(Patrol->GetMesh()->GetMaterial(0) && Patrol->GetMesh()->GetMaterial(0)->GetPathName().Contains(TEXT("/Mobile/M_")), Patrol->PatrolName + TEXT(" uses explicit textured mobile material"));
            UE_LOG(LogTemp, Display, TEXT("ISLAND_MATERIAL: %s %s"), *Patrol->PatrolName, *GetPathNameSafe(Patrol->GetMesh()->GetMaterial(0)));
            UE_LOG(LogTemp, Display, TEXT("ISLAND_MESH_STATE: %s relative=%s feetL=%s feetR=%s"), *Patrol->PatrolName,
                *Patrol->GetMesh()->GetRelativeLocation().ToCompactString(),
                *Patrol->GetMesh()->GetSocketLocation(TEXT("foot_L")).ToCompactString(),
                *Patrol->GetMesh()->GetSocketLocation(TEXT("foot_R")).ToCompactString());
        }
        View(Patrols[0]);
        Phase = 1;
    }
    if (Phase == 1) { View(Patrols[0]); if (Elapsed > 4.f) { Shot(TEXT("01_patrol_seonbi")); Phase = 2; } }
    else if (Phase == 2) { View(Patrols[1]); if (Elapsed > 6.f) { Shot(TEXT("02_patrol_pojol")); Phase = 3; } }
    else if (Phase >= 3 && Phase <= 11) View(Player);
    UJumoLocomotionComponent* Playback = Player ? Player->FindComponentByClass<UJumoLocomotionComponent>() : nullptr;
    if (Phase == 3 && Elapsed > 8.f)
    { Check(Playback && Playback->GetLocomotionState() == TEXT("Idle"), TEXT("Jumo idle animation")); Shot(TEXT("03_jumo_idle")); Phase = 4; }
    if (Phase == 4)
    {
        Player->AddMovementInput(FVector(-1,0,0), 1.f, true);
        if (Elapsed > 10.f)
        { Check(Playback && Playback->GetLocomotionState() == TEXT("Walk"), TEXT("Jumo walk animation")); Shot(TEXT("04_jumo_walk")); Press(TEXT("CharacterRun")); Phase = 5; }
    }
    else if (Phase == 5)
    {
        Player->AddMovementInput(FVector(-1,0,0), 1.f, true);
        if (Elapsed > 12.f)
        {
            Check(Playback && Playback->GetLocomotionState() == TEXT("Run"), TEXT("Run button selects run clip"));
            Check(Player->GetCharacterMovement()->MaxWalkSpeed > BaseSpeed * 1.5f, TEXT("Run button increases movement speed"));
            Shot(TEXT("05_jumo_run")); Press(TEXT("CharacterRun")); Phase = 6;
        }
    }
    else if (Phase == 6 && Elapsed > 13.f)
    {
        Check(FMath::IsNearlyEqual(Player->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed), TEXT("Run OFF restores speed"));
        JumpStartZ = Player->GetActorLocation().Z;
        Press(TEXT("CharacterJump")); Phase = 7;
    }
    else if (Phase == 7)
    {
        MaxJumpRise = FMath::Max(MaxJumpRise, float(Player->GetActorLocation().Z - JumpStartZ));
        if (Elapsed > 13.25f)
        {
            Check(Playback && Playback->GetLocomotionState() == TEXT("Jump"), TEXT("Jump button selects jump clip"));
            Check(Player->GetCharacterMovement()->IsFalling() && MaxJumpRise > 10.f, TEXT("Jump leaves ground"));
            Shot(TEXT("06_jumo_jump")); Press(TEXT("CharacterJump"), true); Phase = 8;
        }
    }
    else if (Phase == 8 && Elapsed > 15.f)
    {
        Check(!Player->GetCharacterMovement()->IsFalling(), TEXT("Jumo landed"));
        Check(Playback && Playback->GetLocomotionState() == TEXT("Idle"), TEXT("Landing returns to idle"));
        Shot(TEXT("07_jumo_landed")); Press(TEXT("WeatherToggle")); Phase = 9;
    }
    else if (Phase == 9 && Elapsed > 16.f)
    {
        Check(Weather->GetWeather() == ESimpleWeather::Rain, TEXT("Weather button switches to rain"));
        USlider* Slider = Cast<USlider>(Controls->GetWidgetFromName(TEXT("RainAmount")));
        Check(Slider != nullptr, TEXT("Rain slider exists"));
        if (Slider)
        {
            Slider->OnValueChanged.Broadcast(0.f); Check(Weather->GetActiveRainDropCount() == 0, TEXT("Rain slider 0%"));
            CheckWeatherSystem(0.f);
            Slider->OnValueChanged.Broadcast(0.5f); Check(Weather->GetActiveRainDropCount() == 200, TEXT("Rain slider 50%"));
            CheckWeatherSystem(0.5f);
            Slider->OnValueChanged.Broadcast(1.f); Check(Weather->GetActiveRainDropCount() == 400, TEXT("Rain slider 100%"));
            CheckWeatherSystem(1.f);
        }
        Phase = 10;
    }
    else if (Phase == 10 && Elapsed > 18.f)
    { Shot(TEXT("08_rain_100")); Phase = 11; }
    else if (Phase == 11 && Elapsed > 20.f)
    { Press(TEXT("WeatherToggle")); Check(Weather->GetWeather() == ESimpleWeather::Clear, TEXT("Weather returns to clear")); CheckWeatherSystem(0.f); Shot(TEXT("09_clear")); PlayerReturnLocation = Player->GetActorLocation(); Phase = 20; }
    else if (Phase == 20)
    {
        AIslandPatrolCharacter* Patrol = Patrols[AttentionPatrolIndex];
        Check(Player->SetActorLocation(Patrol->GetActorLocation() + FVector(200.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics),
            Patrol->PatrolName + TEXT(" approach test position"));
        Player->GetCharacterMovement()->StopMovementImmediately();
        AttentionPhaseStarted = Elapsed;
        Phase = 21;
    }
    else if (Phase == 21 && Elapsed > AttentionPhaseStarted + 1.5f)
    {
        AIslandPatrolCharacter* Patrol = Patrols[AttentionPatrolIndex];
        Check(Patrol->IsFacingPlayer() && Patrol->GetVelocity().Size2D() < 1.f,
            Patrol->PatrolName + TEXT(" stops for nearby player"));
        const FVector TowardPlayer = (Player->GetActorLocation() - Patrol->GetActorLocation()).GetSafeNormal2D();
        Check(FVector::DotProduct(Patrol->GetActorForwardVector(), TowardPlayer) > 0.95f,
            Patrol->PatrolName + TEXT(" faces nearby player"));
        AttentionStopLocation = Patrol->GetActorLocation();
        AttentionReachedPoints = Patrol->GetReachedPatrolPoints();
        Player->SetActorLocation(AttentionStopLocation + FVector(0.f, 300.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->StopMovementImmediately();
        AttentionPhaseStarted = Elapsed;
        Phase = 22;
    }
    else if (Phase == 22 && Elapsed > AttentionPhaseStarted + 1.5f)
    {
        AIslandPatrolCharacter* Patrol = Patrols[AttentionPatrolIndex];
        Check(Patrol->IsFacingPlayer() && FVector::Dist2D(AttentionStopLocation, Patrol->GetActorLocation()) < 2.f
            && Patrol->GetReachedPatrolPoints() == AttentionReachedPoints,
            Patrol->PatrolName + TEXT(" holds route in approach hysteresis band"));
        Check(FVector::DotProduct(Patrol->GetActorForwardVector(),
            (Player->GetActorLocation() - Patrol->GetActorLocation()).GetSafeNormal2D()) > 0.95f,
            Patrol->PatrolName + TEXT(" follows player facing while stopped"));
        View(Patrol);
        Shot(FString::Printf(TEXT("%02d_%s_face_player"), 10 + AttentionPatrolIndex, *Patrol->PatrolName));
        // Move away on the next tick so the screenshot captures the interaction.
        Phase = 23;
    }
    else if (Phase == 23)
    {
        Player->SetActorLocation(AttentionStopLocation + FVector(800.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->StopMovementImmediately();
        AttentionPhaseStarted = Elapsed;
        Phase = 24;
    }
    else if (Phase == 24 && Elapsed > AttentionPhaseStarted + 1.f)
    {
        AIslandPatrolCharacter* Patrol = Patrols[AttentionPatrolIndex];
        Check(!Patrol->IsFacingPlayer() && Patrol->GetVelocity().Size2D() > 10.f
            && FVector::Dist2D(AttentionStopLocation, Patrol->GetActorLocation()) > 30.f,
            Patrol->PatrolName + TEXT(" resumes patrol after player leaves"));
        if (++AttentionPatrolIndex < Patrols.Num()) Phase = 20;
        else
        {
            Player->SetActorLocation(PlayerReturnLocation, false, nullptr, ETeleportType::TeleportPhysics);
            Player->GetCharacterMovement()->StopMovementImmediately();
            PC->SetViewTarget(Player);
            Phase = 12;
        }
    }
    else if (Phase == 12)
    {
        const bool bBothCompleted = Patrols.Num() == 2 && Patrols[0]->GetReachedPatrolPoints() >= 96 && Patrols[1]->GetReachedPatrolPoints() >= 96;
        if (bBothCompleted || Elapsed > 220.f)
        {
            Check(bBothCompleted, TEXT("Both characters completed a full hill loop"));
            UE_LOG(LogTemp, Display, TEXT("ISLAND_VALIDATION_%s: elapsed=%.1f"), bFailed ? TEXT("FAIL") : TEXT("PASS"), Elapsed);
            Phase = 13;
            if (FParse::Param(FCommandLine::Get(), TEXT("IslandValidationExit")))
                FPlatformMisc::RequestExitWithStatus(false, bFailed ? 1 : 0);
        }
    }
}

void AIslandValidationActor::SaveCapture(int32 Width, int32 Height, const TArray<FColor>& Pixels)
{
    if (PendingCapture.IsEmpty()) return;
    TArray64<uint8> Compressed;
    FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
    const bool bSaved = FFileHelper::SaveArrayToFile(Compressed, *PendingCapture);
    Check(bSaved, TEXT("Screenshot saved: ") + PendingCapture);
    UE_LOG(LogTemp, Display, TEXT("ISLAND_CAPTURE_SAVED: %s %dx%d external=%s"), *PendingCapture, Width, Height,
        *IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*PendingCapture));
    PendingCapture.Empty();
}
void AIslandValidationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CaptureHandle.IsValid()) UGameViewportClient::OnScreenshotCaptured().Remove(CaptureHandle);
    Super::EndPlay(EndPlayReason);
}
