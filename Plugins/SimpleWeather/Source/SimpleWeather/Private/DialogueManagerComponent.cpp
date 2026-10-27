#include "DialogueManagerComponent.h"
#include "DialogueComponent.h"
#include "DialogueDataAsset.h"
#include "DialogueWidget.h"
#include "IslandPatrolCharacter.h"
#include "WeatherControlWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SWindow.h"

UDialogueManagerComponent::UDialogueManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UDialogueManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    Player = Cast<ACharacter>(GetOwner());
    Controller = Player.IsValid() ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!Controller.IsValid() || !Controller->IsLocalController()) { SetComponentTickEnabled(false); return; }
    FString Error;
    if (!UDialogueDataAsset::LoadCatalog(this, Catalog, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("DIALOGUE_DATA_ERROR: %s"), *Error);
        SetComponentTickEnabled(false);
        return;
    }
    Widget = CreateWidget<UDialogueWidget>(Controller.Get());
    if (!Widget) { SetComponentTickEnabled(false); return; }
    Widget->SetManager(this);
    Widget->AddToViewport(2000);
    MapLeavingHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UDialogueManagerComponent::OnMapLeaving);
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UDialogueManagerComponent::OnApplicationBackgrounded);
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_READY: %d NPC data records, local-player controls installed"), Catalog.Num());
}

UDialogueDataAsset* UDialogueManagerComponent::ResolveData(AIslandPatrolCharacter* Npc) const
{
    if (!IsValid(Npc) || !Npc->Dialogue) return nullptr;
    if (Npc->Dialogue->Data) return Npc->Dialogue->Data;
    const FName Id = Npc->Dialogue->DialogueId.IsNone() ? FName(*Npc->PatrolName) : Npc->Dialogue->DialogueId;
    const TObjectPtr<UDialogueDataAsset>* Found = Catalog.Find(Id);
    return Found ? Found->Get() : nullptr;
}

void UDialogueManagerComponent::UpdateCandidate()
{
    AIslandPatrolCharacter* Closest = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    if (Player.IsValid() && Controller.IsValid() && Controller->GetPawn() == Player.Get())
    {
        for (TActorIterator<AIslandPatrolCharacter> It(GetWorld()); It; ++It)
        {
            // The existing patrol attention owns stop/face behavior and height checks.
            // Only the prompt uses the entry radius; the NPC retains its 350 cm exit band.
            const float Distance = FVector::DistSquared2D(Player->GetActorLocation(), It->GetActorLocation());
            if (It->IsFacingPlayer() && !It->Dialogue->IsInConversation() && ResolveData(*It)
                && FMath::Abs(Player->GetActorLocation().Z - It->GetActorLocation().Z) <= It->MaxInteractionHeight
                && Distance <= FMath::Square(FMath::Max(0.f, It->ApproachDistance)) && Distance < BestDistance && CanSeeNpc(*It))
            { Closest = *It; BestDistance = Distance; }
        }
    }
    if (Candidate.Get() != Closest)
    {
        Candidate = Closest;
        UE_LOG(LogTemp, Display, TEXT("DIALOGUE_CANDIDATE: %s"), Closest ? *Closest->PatrolName : TEXT("none"));
    }
    if (Widget) Widget->SetCandidate(Closest);
}

bool UDialogueManagerComponent::CanSeeNpc(AIslandPatrolCharacter* Npc) const
{
    if (!IsValid(Npc) || Npc->IsHidden() || !Npc->GetMesh()->IsVisible() || !Controller.IsValid() || !Controller->PlayerCameraManager) return false;
    int32 Width = 0, Height = 0;
    Controller->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return false;
    const FBoxSphereBounds Bounds = Npc->GetMesh()->GetBounds();
    const FVector Head(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z - 12.f);
    const FVector Chest = Npc->GetActorLocation() + FVector(0.f, 0.f, 35.f);
    FVector2D Screen;
    if (!Controller->ProjectWorldLocationToScreen(Head, Screen, true)
        || Screen.X < 12.f || Screen.X > Width - 12.f || Screen.Y < 12.f || Screen.Y > Height - 12.f) return false;
    if (!Controller->ProjectWorldLocationToScreen(Chest, Screen, true)
        || Screen.X < 12.f || Screen.X > Width - 12.f || Screen.Y < 12.f || Screen.Y > Height - 12.f) return false;
    const FVector Eye = Controller->PlayerCameraManager->GetCameraLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DialogueVisibility), true);
    Query.AddIgnoredActor(Npc);
    FCollisionObjectQueryParams Occluders;
    Occluders.AddObjectTypesToQuery(ECC_WorldStatic);
    Occluders.AddObjectTypesToQuery(ECC_WorldDynamic);
    Occluders.AddObjectTypesToQuery(ECC_Pawn);
    Occluders.AddObjectTypesToQuery(ECC_PhysicsBody);
    // Do not ignore the player: a target completely behind the player's body
    // should not look like an interaction with an empty patch of screen.
    return !GetWorld()->LineTraceTestByObjectType(Eye, Head, Occluders, Query)
        && !GetWorld()->LineTraceTestByObjectType(Eye, Chest, Occluders, Query);
}

void UDialogueManagerComponent::StagePlayer(AIslandPatrolCharacter* Npc)
{
    bPlayerWasStaged = false;
    if (!ActiveData->Presentation.bStagePlayer || !Player.IsValid()) return;
    const FDialoguePresentation& Profile = ActiveData->Presentation;
    const FVector Direction = (Player->GetActorLocation() - Npc->GetActorLocation()).GetSafeNormal2D();
    FVector Desired = Npc->GetActorLocation() + Direction * Profile.PlayerDistanceCm;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DialogueStage), false);
    Query.AddIgnoredActor(Player.Get()); Query.AddIgnoredActor(Npc);
    FCollisionObjectQueryParams GroundObjects;
    GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
    FHitResult Ground;
    const bool bGround = GetWorld()->LineTraceSingleByObjectType(Ground, Desired + FVector(0.f, 0.f, 300.f),
        Desired - FVector(0.f, 0.f, 500.f), GroundObjects, Query);
    const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    if (bGround && Ground.ImpactNormal.Z >= Player->GetCharacterMovement()->GetWalkableFloorZ()
        && FMath::Abs(Ground.ImpactPoint.Z + HalfHeight - Player->GetActorLocation().Z) <= Player->GetCharacterMovement()->MaxStepHeight)
    {
        Desired.Z = Ground.ImpactPoint.Z + HalfHeight + 2.f;
        const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, HalfHeight - 4.f));
        FHitResult Obstacle;
        const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(Desired, FQuat::Identity, ECC_Pawn, Shape, Query)
            || GetWorld()->SweepSingleByChannel(Obstacle, Player->GetActorLocation() + FVector(0.f, 0.f, 4.f),
                Desired + FVector(0.f, 0.f, 4.f), FQuat::Identity, ECC_Pawn, Shape, Query);
        if (!bBlocked)
        {
            Player->SetActorLocation(Desired, false, nullptr, ETeleportType::TeleportPhysics);
            bPlayerWasStaged = true;
        }
    }
    const FRotator Facing(0.f, (Npc->GetActorLocation() - Player->GetActorLocation()).Rotation().Yaw, 0.f);
    Player->SetActorRotation(Facing);
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_STAGE: npc=%s staged=%d distance=%.0f"), *Npc->PatrolName, bPlayerWasStaged,
        FVector::Dist2D(Player->GetActorLocation(), Npc->GetActorLocation()));
}

bool UDialogueManagerComponent::StartConversation()
{
    if (bActive || CameraReturnRemaining > 0.f || !Widget || !Player.IsValid() || !Controller.IsValid()) return false;
    UpdateCandidate();
    AIslandPatrolCharacter* Npc = Candidate.Get();
    UDialogueDataAsset* Data = ResolveData(Npc);
    FString Error;
    if (!Npc || !Data || !Data->Validate(Error) || Player->GetCharacterMovement()->IsFalling()) return false;
    ConversationCamera = GetWorld()->SpawnActor<ACameraActor>();
    if (!ConversationCamera || !Npc->Dialogue->Acquire(this))
    { if (ConversationCamera) ConversationCamera->Destroy(); ConversationCamera = nullptr; return false; }
    ActiveNpc = Npc;
    ActiveData = Data;
    CurrentNode = Data->EntryNode;
    PreviousViewTarget = Controller->GetViewTarget();
    PreviousControlRotation = Controller->GetControlRotation();
    PreviousPlayerTransform = Player->GetActorTransform();
    StagePlayer(Npc);
    bPreviousCursor = Controller->bShowMouseCursor;
    if (FSlateApplication::IsInitialized()) PreviousFocus = FSlateApplication::Get().GetKeyboardFocusedWidget();
    UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
    PreviousMovementMode = Movement->MovementMode;
    PreviousCustomMovementMode = Movement->CustomMovementMode;
    bPreviousPawnInputEnabled = Player->InputEnabled();
    bActive = true;
    Player->StopJumping();
    Player->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();
    Movement->DisableMovement();
    Player->DisableInput(Controller.Get());
    Controller->SetIgnoreMoveInput(true);
    Controller->SetIgnoreLookInput(true);
    bAddedInputLocks = true;

    const FVector Between = Player->GetActorLocation() - Npc->GetActorLocation();
    const FVector Facing = Between.GetSafeNormal2D();
    const FDialoguePresentation& Profile = Data->Presentation;
    const FVector Side = FVector::CrossProduct(FVector::UpVector, Facing);
    const FVector Focus = FMath::Lerp(Player->GetActorLocation(), Npc->GetActorLocation(), 0.6f) + FVector(0.f, 0.f, Profile.FocusHeightCm);
    FVector Eye = Npc->GetActorLocation() + Facing * Profile.CameraDistanceCm + Side * Profile.CameraSideOffsetCm + FVector(0.f, 0.f, Profile.CameraHeightCm);
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DialogueCamera), false);
    Query.AddIgnoredActor(Player.Get()); Query.AddIgnoredActor(Npc);
    if (GetWorld()->SweepSingleByChannel(Hit, Focus, Eye, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Query))
        Eye = Hit.Location + Hit.Normal * 14.f;
    ConversationCamera->SetActorLocation(Eye);
    ConversationCamera->SetActorRotation((Focus - Eye).Rotation());
    ConversationCamera->GetCameraComponent()->SetFieldOfView(Profile.FieldOfView);
    ConversationCamera->GetCameraComponent()->bConstrainAspectRatio = false;
    Controller->SetViewTargetWithBlend(ConversationCamera, 0.4f, VTBlend_Cubic, 0.f, true);
    SetOtherControlsHidden(true);
#if PLATFORM_DESKTOP
    Controller->bShowMouseCursor = true;
#endif
    PresentNode();
    Widget->SetKeyboardFocus();
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_STARTED: npc=%s previousView=%s"), *Data->NpcId.ToString(), *GetNameSafe(PreviousViewTarget.Get()));
    return true;
}

void UDialogueManagerComponent::PresentNode()
{
    const FDialogueNode* Node = ActiveData ? ActiveData->FindNode(CurrentNode) : nullptr;
    if (!Node) { EndConversation(); return; }
    Widget->ShowNode(ActiveData->DisplayName, *Node);
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_NODE: npc=%s node=%s text=%s"), *ActiveData->NpcId.ToString(), *CurrentNode.ToString(), *Node->Text.ToString());
    OnNodeChanged.Broadcast(ActiveData->NpcId, CurrentNode);
}

void UDialogueManagerComponent::Choose(int32 ChoiceIndex)
{
    const FDialogueNode* Node = bActive && ActiveData ? ActiveData->FindNode(CurrentNode) : nullptr;
    if (!Node || !Node->Choices.IsValidIndex(ChoiceIndex)) return;
    const FDialogueChoice Choice = Node->Choices[ChoiceIndex];
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_CHOICE: npc=%s choice=%s"), *ActiveData->NpcId.ToString(), *Choice.Id.ToString());
    OnChoiceSelected.Broadcast(ActiveData->NpcId, Choice.Id, Choice.EventTag);
    if (!bActive) return; // External event handlers may close the session or remove the NPC.
    if (Choice.bEndConversation) EndConversation();
    else { CurrentNode = Choice.NextNode; PresentNode(); }
}

void UDialogueManagerComponent::SetOtherControlsHidden(bool bHide)
{
    if (bHide)
    {
        for (TObjectIterator<UUserWidget> It; It; ++It)
        {
            const bool bWeather = It->IsA<UWeatherControlWidget>();
            const bool bLegacyTouch = It->GetWidgetFromName(TEXT("Thumbstick_Move")) && It->GetWidgetFromName(TEXT("Btn_Jump"));
            if ((!bWeather && !bLegacyTouch) || It->GetWorld() != GetWorld() || !It->IsInViewport()
                || (It->GetOwningPlayer() && It->GetOwningPlayer() != Controller.Get())) continue;
            HiddenWidgets.Add({ *It, It->GetVisibility(), It->GetIsEnabled() });
            It->SetIsEnabled(false);
            It->SetVisibility(ESlateVisibility::Collapsed);
        }
        // Preserve actual Slate visibility, including an already-hidden joystick.
        // Do not replace the controller's touch interface or reset its configuration.
        if (Controller.IsValid() && Controller->GetWorld()->GetGameViewport())
        {
            const TSharedPtr<SWindow> Window = Controller->GetWorld()->GetGameViewport()->GetWindow();
            TFunction<void(TSharedRef<SWidget>)> Visit = [&](TSharedRef<SWidget> SlateWidget)
            {
                if (SlateWidget->GetTypeAsString().Contains(TEXT("VirtualJoystick")))
                {
                    HiddenJoysticks.Add({ SlateWidget, SlateWidget->GetVisibility() });
                    // Finish any held joystick fingers before hiding it, so a release
                    // during the modal session cannot leave a latched movement axis.
                    for (uint32 Pointer = 0; Pointer < 10; ++Pointer)
                    {
                        const FPointerEvent Release(Pointer, FVector2D::ZeroVector, FVector2D::ZeroVector,
                            FTouchKeySet::EmptySet, EKeys::Invalid, 0.f, FModifierKeysState());
                        SlateWidget->OnTouchEnded(SlateWidget->GetCachedGeometry(), Release);
                    }
                    SlateWidget->SetVisibility(EVisibility::Collapsed);
                    return;
                }
                FChildren* Children = SlateWidget->GetChildren();
                for (int32 Index = 0; Children && Index < Children->Num(); ++Index) Visit(Children->GetChildAt(Index));
            };
            if (Window) Visit(Window->GetContent());
        }
        if (FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseAllPointerCapture();
        if (Controller.IsValid() && Controller->PlayerInput) Controller->PlayerInput->FlushPressedKeys();
    }
    else
    {
        for (const FHiddenWidget& Saved : HiddenWidgets)
            if (Saved.Widget.IsValid()) { Saved.Widget->SetVisibility(Saved.Visibility); Saved.Widget->SetIsEnabled(Saved.bEnabled); }
        HiddenWidgets.Reset();
        for (const FHiddenJoystick& Saved : HiddenJoysticks)
            if (const TSharedPtr<SWidget> SlateWidget = Saved.Widget.Pin()) SlateWidget->SetVisibility(Saved.Visibility);
        HiddenJoysticks.Reset();
    }
}

void UDialogueManagerComponent::RestoreCamera()
{
    if (Controller.IsValid())
    {
        AActor* Target = PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : Player.Get();
        if (Target) Controller->SetViewTarget(Target);
    }
    if (ConversationCamera) ConversationCamera->Destroy();
    ConversationCamera = nullptr;
    CameraReturnRemaining = 0.f;
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_CAMERA_RESTORED: target=%s"), Controller.IsValid() ? *GetNameSafe(Controller->GetViewTarget()) : TEXT("none"));
}

void UDialogueManagerComponent::EndConversation(bool bImmediate)
{
    if (!bActive) { if (bImmediate && ConversationCamera) RestoreCamera(); return; }
    const FName NpcId = ActiveData ? ActiveData->NpcId : NAME_None;
    bActive = false; // Cleanup is idempotent, including callbacks during widget or NPC destruction.
    if (ActiveNpc.IsValid() && ActiveNpc->Dialogue) ActiveNpc->Dialogue->Release(this);
    if (Widget) Widget->HideConversation();
    if (Player.IsValid())
    {
        Player->StopJumping();
        Player->ConsumeMovementInputVector();
        if (ActiveData && ActiveData->Presentation.bRestorePlayerTransform && ActiveData->Presentation.bStagePlayer)
            Player->SetActorTransform(PreviousPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->SetMovementMode(PreviousMovementMode, PreviousCustomMovementMode);
        if (bPreviousPawnInputEnabled) Player->EnableInput(nullptr);
    }
    if (Controller.IsValid())
    {
        if (bAddedInputLocks) { Controller->SetIgnoreMoveInput(false); Controller->SetIgnoreLookInput(false); }
        Controller->SetControlRotation(PreviousControlRotation);
        Controller->bShowMouseCursor = bPreviousCursor;
        AActor* Target = PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : Player.Get();
        if (Target) Controller->SetViewTargetWithBlend(Target, bImmediate ? 0.f : 0.4f, VTBlend_Cubic, 0.f, true);
    }
    bAddedInputLocks = false;
    SetOtherControlsHidden(false);
    if (FSlateApplication::IsInitialized())
    {
        if (const TSharedPtr<SWidget> Focus = PreviousFocus.Pin()) FSlateApplication::Get().SetKeyboardFocus(Focus);
        else UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }
    PreviousFocus.Reset();
    ActiveNpc.Reset(); ActiveData = nullptr; CurrentNode = NAME_None; Candidate.Reset();
    if (bImmediate) RestoreCamera(); else CameraReturnRemaining = 0.45f;
    UE_LOG(LogTemp, Display, TEXT("DIALOGUE_ENDED: npc=%s pawnInput=%d movement=%d moveIgnored=%d lookIgnored=%d"), *NpcId.ToString(),
        Player.IsValid() && Player->InputEnabled(), Player.IsValid() ? int32(Player->GetCharacterMovement()->MovementMode) : -1,
        Controller.IsValid() && Controller->IsMoveInputIgnored(), Controller.IsValid() && Controller->IsLookInputIgnored());
    OnConversationEnded.Broadcast(NpcId);
}

void UDialogueManagerComponent::TickComponent(float DeltaTime, ELevelTick Type, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(DeltaTime, Type, Tick);
    if (!Widget || !Controller.IsValid()) { if (bActive) EndConversation(true); return; }
    if (bActive)
    {
        if (!ActiveNpc.IsValid() || !Player.IsValid() || Controller->GetPawn() != Player.Get() || !Widget->IsInViewport())
        { EndConversation(true); return; }
        Player->ConsumeMovementInputVector();
        Player->StopJumping();
        if (Controller->WasInputKeyJustPressed(EKeys::Escape)) EndConversation();
    }
    else if (CameraReturnRemaining > 0.f)
    {
        CameraReturnRemaining -= DeltaTime;
        if (CameraReturnRemaining <= 0.f) RestoreCamera();
    }
    else
    {
        UpdateCandidate();
        if (Controller->WasInputKeyJustPressed(EKeys::E)) StartConversation();
    }
}

void UDialogueManagerComponent::OnMapLeaving(const FString& MapName) { EndConversation(true); }
void UDialogueManagerComponent::OnApplicationBackgrounded() { EndConversation(true); }
void UDialogueManagerComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    EndConversation(true);
    FCoreUObjectDelegates::PreLoadMap.Remove(MapLeavingHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    if (Widget) { Widget->SetManager(nullptr); Widget->RemoveFromParent(); Widget = nullptr; }
    Super::EndPlay(Reason);
}
