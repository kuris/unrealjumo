#include "SwimmingValidationActor.h"
#include "FairySwimmingComponent.h"
#include "JumoLocomotionComponent.h"
#include "WeatherControlWidget.h"
#include "Components/Button.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"

ASwimmingValidationActor::ASwimmingValidationActor() { PrimaryActorTick.bCanEverTick = true; }

void ASwimmingValidationActor::Check(bool bCondition, const FString& Description)
{
    if (bCondition) { UE_LOG(LogTemp, Display, TEXT("SWIM_CHECK_PASS: %s"), *Description); }
    else { bFailed = true; UE_LOG(LogTemp, Error, TEXT("SWIM_CHECK_FAIL: %s"), *Description); }
}

void ASwimmingValidationActor::Shot(const FString& Name)
{
    PendingCapture = Output / (Name + TEXT(".png"));
    FScreenshotRequest::RequestScreenshot(PendingCapture, true, false);
}

void ASwimmingValidationActor::SaveCapture(int32 Width, int32 Height, const TArray<FColor>& Pixels)
{
    if (PendingCapture.IsEmpty()) return;
    TArray64<uint8> Compressed;
    FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
    Check(FFileHelper::SaveArrayToFile(Compressed, *PendingCapture), TEXT("Capture saved: ") + PendingCapture);
    PendingCapture.Reset();
}

void ASwimmingValidationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    Elapsed += DeltaSeconds;
    StageTime += DeltaSeconds;
    if (Elapsed > 65.f) { Check(false, TEXT("Swimming test timeout")); Finish(); return; }
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (Phase == 0)
    {
        if (Elapsed < 4.f) return;
        Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        Swimming = Player ? Player->FindComponentByClass<UFairySwimmingComponent>() : nullptr;
        Playback = Player ? Player->FindComponentByClass<UJumoLocomotionComponent>() : nullptr;
        Check(PC && Player && Swimming && Playback && Playback->HasSwimmingAnimations(), TEXT("Fairy swimming component and two compatible clips loaded"));
        if (!PC || !Player || !Swimming || !Playback || !Playback->HasSwimmingAnimations()) { Finish(); return; }
        FParse::Value(FCommandLine::Get(), TEXT("SwimmingValidationOutput="), Output);
        if (Output.IsEmpty()) Output = FPaths::ProjectSavedDir() / TEXT("SwimmingValidation");
        IFileManager::Get().MakeDirectory(*Output, true);
        CaptureHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &ASwimmingValidationActor::SaveCapture);
        OriginalTransform = Player->GetActorTransform();
        OriginalView = PC->GetViewTarget();
        auto* Move = Player->GetCharacterMovement();
        WalkingSpeed = Move->MaxWalkSpeed;
        OriginalSwimSpeed = Move->MaxSwimSpeed;
        OriginalAutoEnterDepth = Swimming->EnterDepth;
        OriginalBuoyancy = Move->Buoyancy;
        bOriginalVolumeUpdates = Player->GetCapsuleComponent()->GetShouldUpdatePhysicsVolume();
        Camera = GetWorld()->SpawnActor<ACameraActor>();
        PC->SetViewTarget(Camera);
        Move->StopMovementImmediately();
        Player->SetActorLocationAndRotation(FVector(3000,-13000, -53.f + Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3.f), FRotator(0,-90,0), false, nullptr, ETeleportType::TeleportPhysics);
        Move->SetMovementMode(MOVE_Falling);
        Phase = 1; StageTime = 0.f;
    }
    if (!Player) return;
    auto* Move = Player->GetCharacterMovement();
    const FVector Center = Player->GetActorLocation();
    const FVector Eye = Center + FVector(420,420,230);
    Camera->SetActorLocation(Eye);
    Camera->SetActorRotation((Center + FVector(0,0,25)-Eye).Rotation());
    if (Phase == 1 && StageTime > 2.f)
    {
        Check(!Swimming->IsSurfaceSwimming() && Move->IsMovingOnGround(), TEXT("Shallow water retains walking"));
        Check(!Swimming->CanStartSwimming() && !Swimming->StartSwimming(), TEXT("Swimming button unavailable in water that is too shallow"));
        Shot(TEXT("01_shallow_walking"));
        SwimStart = Player->GetActorLocation();
        Phase = 2; StageTime = 0.f;
    }
    else if (Phase == 2)
    {
        Player->AddMovementInput(FVector(0,-1,0));
        if (StageTime > 6.f)
        {
            Check(Swimming->IsSurfaceSwimming() && Move->IsSwimming(), TEXT("Walking into deeper ocean enters native swimming"));
            Check(Playback->GetLocomotionState()==TEXT("SwimForward"), TEXT("Forward swimming animation plays"));
            Check(FVector::Dist2D(SwimStart, Center)>300.f, TEXT("Swimming input moves the capsule"));
            Check(FMath::IsNearlyEqual(Move->MaxSwimSpeed, Swimming->SwimSpeed), TEXT("Swimming speed is independent of running"));
            Shot(TEXT("02_swim_forward"));
            Phase=3; StageTime=0.f;
        }
    }
    else if (Phase == 3 && StageTime > 4.f)
    {
        Check(Swimming->IsSurfaceSwimming() && Playback->GetLocomotionState()==TEXT("SwimIdle"), TEXT("Released input treads water"));
        Check(FMath::Abs(Center.Z - (Swimming->GetSurfaceHeight()+Swimming->TreadSurfaceOffset))<20.f, TEXT("Idle stays at water surface"));
        Check(FMath::IsNearlyEqual(Move->MaxWalkSpeed, WalkingSpeed), TEXT("Walking speed preserved during swimming"));
        Shot(TEXT("03_treading_water"));
        Phase=4; StageTime=0.f;
    }
    else if (Phase == 4 && StageTime > 1.f)
    {
        Move->DisableMovement();
        Phase=5; StageTime=0.f;
    }
    else if (Phase == 5 && StageTime > 0.5f)
    {
        Check(!Swimming->IsSurfaceSwimming() && Move->MovementMode==MOVE_None, TEXT("External input lock cancels swimming without undoing the lock"));
        Check(Player->GetCapsuleComponent()->GetShouldUpdatePhysicsVolume()==bOriginalVolumeUpdates
            && FMath::IsNearlyEqual(Move->Buoyancy,OriginalBuoyancy)
            && FMath::IsNearlyEqual(Move->MaxSwimSpeed,OriginalSwimSpeed), TEXT("Cancellation restores volume policy and movement settings"));
        Move->SetMovementMode(MOVE_Falling);
        Phase=6; StageTime=0.f;
    }
    else if (Phase == 6 && StageTime > 2.f)
    {
        Check(Swimming->IsSurfaceSwimming() && Move->IsSwimming(), TEXT("Swimming resumes after external lock is released"));
        Phase=7; StageTime=0.f;
    }
    else if (Phase == 7)
    {
        Player->AddMovementInput(FVector(0,1,0));
        if (!Swimming->IsSurfaceSwimming() && Move->IsMovingOnGround())
        {
            Check(true,TEXT("Swimming back to shallow shore resumes walking"));
            Check(FMath::IsNearlyEqual(Move->MaxSwimSpeed,OriginalSwimSpeed)
                && FMath::IsNearlyEqual(Move->Buoyancy,OriginalBuoyancy),TEXT("Shore exit restores original movement settings"));
            Shot(TEXT("04_shore_return"));
            Phase=8; StageTime=0.f;
        }
    }
    else if (Phase == 8 && StageTime > 1.f)
    {
        Player->Jump();
        Phase=9; StageTime=0.f;
    }
    else if (Phase == 9 && StageTime > 0.2f)
    {
        Check(Move->IsFalling() && Move->Velocity.Z>0, TEXT("Jump works after leaving water"));
        Player->StopJumping();
        Phase=10; StageTime=0.f;
    }
    else if (Phase == 10 && StageTime > 2.f)
    {
        Swimming->EnterDepth = 10000.f; // Isolate the manual route from automatic entry.
        Move->StopMovementImmediately();
        Player->SetActorLocation(FVector(3000,-14000,-35),false,nullptr,ETeleportType::TeleportPhysics);
        Move->SetMovementMode(MOVE_Falling);
        Phase=11; StageTime=0.f;
    }
    else if (Phase == 11 && StageTime > 0.5f)
    {
        Check(Swimming->CanStartSwimming() && !Swimming->IsSurfaceSwimming(), TEXT("Swim button available before automatic entry"));
        UButton* Button=nullptr;
        for (TObjectIterator<UWeatherControlWidget> It; It; ++It)
            if (It->GetWorld()==GetWorld() && It->IsInViewport()) Button=Cast<UButton>(It->GetWidgetFromName(TEXT("CharacterSwim")));
        Check(Button && Button->IsVisible() && Button->GetIsEnabled(),TEXT("Conditional swim button visible and enabled"));
        Shot(TEXT("05_manual_swim_button"));
        Phase=12; StageTime=0.f;
    }
    else if (Phase == 12 && StageTime > 0.3f)
    {
        for (TObjectIterator<UWeatherControlWidget> It; It; ++It)
            if (It->GetWorld()==GetWorld() && It->IsInViewport())
                if (UButton* Button=Cast<UButton>(It->GetWidgetFromName(TEXT("CharacterSwim")))) Button->OnPressed.Broadcast();
        Check(Swimming->IsSurfaceSwimming() && Move->IsSwimming(),TEXT("Swim button starts native swimming"));
        Swimming->EnterDepth=OriginalAutoEnterDepth;
        Phase=13; StageTime=0.f;
    }
    else if (Phase == 13 && StageTime > 1.f) Finish();
}

void ASwimmingValidationActor::Finish()
{
    if (bFinished) return;
    bFinished=true;
    if (Player)
    {
        if (Swimming) Swimming->EnterDepth=OriginalAutoEnterDepth;
        Player->StopJumping();
        Player->GetCharacterMovement()->StopMovementImmediately();
        Player->SetActorTransform(OriginalTransform,false,nullptr,ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (auto* PC=Cast<APlayerController>(Player->GetController())) if (OriginalView.IsValid()) PC->SetViewTarget(OriginalView.Get());
    }
    UE_LOG(LogTemp,Display,TEXT("SWIMMING_VALIDATION_%s"),bFailed?TEXT("FAIL"):TEXT("PASS"));
    if (FParse::Param(FCommandLine::Get(),TEXT("SwimmingValidationExit"))) FPlatformMisc::RequestExitWithStatus(false,bFailed?1:0);
}

void ASwimmingValidationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UGameViewportClient::OnScreenshotCaptured().Remove(CaptureHandle);
    if (Camera) Camera->Destroy();
    Super::EndPlay(EndPlayReason);
}
