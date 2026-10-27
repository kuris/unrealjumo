#include "IslandValidationActor.h"
#include "DialogueComponent.h"
#include "DialogueDataAsset.h"
#include "DialogueManagerComponent.h"
#include "DialogueWidget.h"
#include "IslandPatrolCharacter.h"
#include "SimpleWeatherActor.h"
#include "WeatherControlWidget.h"
#include "Components/Button.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AIslandValidationActor::TickDialogueValidation()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (Phase == 30)
    {
        DialogueManager = Player->FindComponentByClass<UDialogueManagerComponent>();
        Check(DialogueManager && DialogueManager->GetDialogueWidget(), TEXT("Dialogue manager and packaged JSON catalog loaded"));
        if (!DialogueManager || !DialogueManager->GetDialogueWidget()) { Phase = 12; return; }
        DialoguePlayerHome = Player->GetActorLocation();
        DialogueControlHome = PC->GetControlRotation();
        DialogueControlsVisibility = static_cast<uint8>(Controls->GetVisibility());
        bDialogueControlsEnabled = Controls->GetIsEnabled();
        Phase = 31;
    }
    UDialogueWidget* UI = DialogueManager->GetDialogueWidget();
    AIslandPatrolCharacter* Npc = Patrols[DialogueNpcIndex];
    UDialogueDataAsset* Data = DialogueManager->ResolveData(Npc);
    if (!Data) { Check(false, TEXT("Dialogue NPC data exists")); Phase = 12; return; }
    auto MovePlayer = [&](FVector Location)
    {
        Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->StopMovementImmediately();
        Player->ConsumeMovementInputVector();
        PC->SetControlRotation(FRotator::ZeroRotator);
        Player->SetActorRotation(FRotator::ZeroRotator);
    };
    auto Click = [&](FName Name)
    {
        UButton* Button = Cast<UButton>(UI->GetWidgetFromName(Name));
        Check(Button != nullptr, TEXT("Dialogue button bound: ") + Name.ToString());
        if (Button) Button->OnClicked.Broadcast();
    };
    auto CheckNode = [&](FName Expected)
    {
        const FDialogueNode* Node = Data->FindNode(Expected);
        UTextBlock* Text = Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("DialogueSpeech")));
        UTextBlock* Name = Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("DialogueSpeaker")));
        Check(DialogueManager->GetCurrentNodeId() == Expected && Node && Text && Text->GetText().EqualTo(Node->Text)
            && Name && Name->GetText().EqualTo(Data->DisplayName), Npc->PatrolName + TEXT(" dialogue text/branch matches data: ") + Expected.ToString());
    };
    auto ClickAfterCapture = [&](FName Name, int32 Count = 1)
    {
        DialoguePendingButton = Name; DialoguePendingClickCount = Count; DialoguePendingClickAt = Elapsed + 0.2f;
    };
    if (!DialoguePendingButton.IsNone() && Elapsed >= DialoguePendingClickAt)
    {
        const FName Button = DialoguePendingButton;
        DialoguePendingButton = NAME_None;
        for (int32 Index = 0; Index < DialoguePendingClickCount; ++Index) Click(Button);
    }
    auto CheckRestored = [&]()
    {
        Check(!DialogueManager->IsConversationActive() && !UI->IsConversationVisible() && !Npc->Dialogue->IsInConversation(),
            Npc->PatrolName + TEXT(" dialogue session closed"));
        Check(Player->InputEnabled() && Player->GetCharacterMovement()->MovementMode == MOVE_Walking
            && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("Dialogue restores movement, jump and look input"));
        Check(PC->GetViewTarget() == DialoguePreviousView && PC->GetControlRotation().Equals(DialogueExpectedControl, 0.01f),
            TEXT("Dialogue restores exact view target and control rotation"));
        Check(FVector::Dist2D(Player->GetActorLocation(), DialoguePlayerBefore.GetLocation()) < 1.f
            && Player->GetActorRotation().Equals(DialoguePlayerBefore.Rotator(), 0.1f), TEXT("Dialogue restores original player placement"));
        Check(Controls->GetVisibility() == static_cast<ESlateVisibility>(DialogueControlsVisibility)
            && Controls->GetIsEnabled() == bDialogueControlsEnabled, TEXT("Dialogue restores weather and action controls"));
    };
    if (Phase == 31)
    {
        MovePlayer(Npc->GetActorLocation() + FVector(-200.f, 0.f, 0.f));
        PC->SetViewTarget(Player);
        DialogueStageTime = Elapsed; Phase = 54;
    }
    else if (Phase == 54 && Elapsed > DialogueStageTime + 0.7f)
    {
        Check(!UI->IsPromptVisible(), Npc->PatrolName + TEXT(" no ghost prompt when NPC is occluded by player"));
        MovePlayer(Npc->GetActorLocation() + FVector(-140.f, -100.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 32;
    }
    else if (Phase == 32 && Elapsed > DialogueStageTime + 1.f)
    {
        Check(DialogueManager->GetCandidate() == Npc && UI->IsPromptVisible(), Npc->PatrolName + TEXT(" dialogue prompt appears inside 250cm"));
        Shot(Npc->PatrolName + TEXT("_dialogue_approach"));
        DialogueStageTime = Elapsed; Phase = 33;
    }
    else if (Phase == 33 && Elapsed > DialogueStageTime + 0.25f)
    {
        const FVector Eye = PC->PlayerCameraManager->GetCameraLocation();
        Camera->SetActorLocation(Eye);
        Camera->SetActorRotation((Npc->GetActorLocation() - Eye).Rotation() + FRotator(0.f, 180.f, 0.f));
        PC->SetViewTarget(Camera);
        DialogueStageTime = Elapsed; Phase = 52;
    }
    else if (Phase == 52 && Elapsed > DialogueStageTime + 0.4f)
    {
        Check(!UI->IsPromptVisible() && !DialogueManager->GetCandidate(), Npc->PatrolName + TEXT(" prompt hides when looking away from NPC"));
        Shot(Npc->PatrolName + TEXT("_look_away_no_prompt"));
        DialogueStageTime = Elapsed; Phase = 55;
    }
    else if (Phase == 55 && Elapsed > DialogueStageTime + 0.25f)
    {
        PC->SetViewTarget(Player);
        PC->SetControlRotation(FRotator::ZeroRotator);
        Npc->SetActorHiddenInGame(true);
        DialogueStageTime = Elapsed; Phase = 53;
    }
    else if (Phase == 53 && Elapsed > DialogueStageTime + 0.3f)
    {
        Check(!UI->IsPromptVisible() && !DialogueManager->GetCandidate(), Npc->PatrolName + TEXT(" hidden NPC cannot offer dialogue"));
        Npc->SetActorHiddenInGame(false);
        MovePlayer(Npc->GetActorLocation() + FVector(-300.f, 0.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 34;
    }
    else if (Phase == 34 && Elapsed > DialogueStageTime + 0.4f)
    {
        Check(!UI->IsPromptVisible() && !DialogueManager->GetCandidate() && Npc->IsFacingPlayer(),
            Npc->PatrolName + TEXT(" prompt hides outside 250cm; existing 350cm attention retained"));
        MovePlayer(Npc->GetActorLocation() + FVector(-140.f, -100.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 35;
    }
    else if (Phase == 35 && Elapsed > DialogueStageTime + 0.4f)
    {
        DialoguePreviousView = PC->GetViewTarget(); DialogueExpectedControl = PC->GetControlRotation();
        DialoguePlayerBefore = Player->GetActorTransform(); DialogueNpcBefore = Npc->GetActorLocation();
        Click(TEXT("DialogueTalk"));
        DialogueStoppedPoints = Npc->GetReachedPatrolPoints();
        DialogueStageTime = Elapsed; Phase = 36;
    }
    else if (Phase == 36 && Elapsed > DialogueStageTime + 0.7f)
    {
        Check(DialogueManager->IsConversationActive() && UI->IsConversationVisible() && Npc->Dialogue->IsInConversation(),
            Npc->PatrolName + TEXT(" dialogue starts from bound talk button"));
        CheckNode(Data->EntryNode);
        Check(Player->GetCharacterMovement()->MovementMode == MOVE_None && !Player->InputEnabled()
            && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(), TEXT("Dialogue blocks player movement and jump"));
        Check(Npc->GetReachedPatrolPoints() == DialogueStoppedPoints && Npc->GetVelocity().Size2D() < 1.f,
            TEXT("Dialogue freezes NPC route without losing waypoint"));
        Check(Controls->GetVisibility() == ESlateVisibility::Collapsed && !Controls->GetIsEnabled(), TEXT("Dialogue hides and disables weather/action UI"));
        Check(PC->GetViewTarget() != DialoguePreviousView, TEXT("Dialogue camera blend completed"));
        Check(FMath::IsNearlyEqual(FVector::Dist2D(Player->GetActorLocation(), Npc->GetActorLocation()), Data->Presentation.PlayerDistanceCm, 2.f)
            && FVector::Dist(Npc->GetActorLocation(), DialogueNpcBefore) < 1.f, TEXT("Conversation profile stages player distance without moving NPC"));
        FVector2D PlayerScreen, NpcScreen;
        Check(PC->ProjectWorldLocationToScreen(Player->GetActorLocation() + FVector(0.f, 0.f, 60.f), PlayerScreen)
            && PC->ProjectWorldLocationToScreen(Npc->GetActorLocation() + FVector(0.f, 0.f, 60.f), NpcScreen)
            && PlayerScreen.X < NpcScreen.X, TEXT("Conversation profile frames player left and NPC right"));
        Shot(Npc->PatrolName + TEXT("_dialogue_greeting"));
        ClickAfterCapture(TEXT("DialogueChoice0"));
        DialogueStageTime = Elapsed; Phase = 37;
    }
    else if (Phase == 37 && Elapsed > DialogueStageTime + 0.6f)
    {
        CheckNode(Data->FindNode(Data->EntryNode)->Choices[0].NextNode);
        Shot(Npc->PatrolName + TEXT("_dialogue_branch_1"));
        const FName Before = DialogueManager->GetCurrentNodeId();
        DialogueManager->Choose(-1); DialogueManager->Choose(999);
        Check(DialogueManager->GetCurrentNodeId() == Before && DialogueManager->IsConversationActive(), TEXT("Dialogue rejects invalid choice indexes"));
        ClickAfterCapture(TEXT("DialogueChoice1"), 2); // Back to greeting, then second branch.
        DialogueStageTime = Elapsed; Phase = 38;
    }
    else if (Phase == 38 && Elapsed > DialogueStageTime + 0.6f)
    {
        CheckNode(Data->FindNode(Data->EntryNode)->Choices[1].NextNode);
        Shot(Npc->PatrolName + TEXT("_dialogue_branch_2"));
        ClickAfterCapture(TEXT("DialogueClose"));
        DialogueStageTime = Elapsed; Phase = 39;
    }
    else if (Phase == 39 && Elapsed > DialogueStageTime + 0.85f)
    {
        CheckRestored();
        MovePlayer(Npc->GetActorLocation() + FVector(-800.f, 0.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 40;
    }
    else if (Phase == 40 && Elapsed > DialogueStageTime + 1.f)
    {
        Check(!Npc->IsFacingPlayer() && Npc->GetVelocity().Size2D() > 10.f, Npc->PatrolName + TEXT(" resumes patrol after dialogue and distance exit"));
        MovePlayer(Npc->GetActorLocation() + FVector(-160.f, -120.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 41;
    }
    else if (Phase == 41 && Elapsed > DialogueStageTime + 0.5f)
    {
        DialoguePreviousView = PC->GetViewTarget(); DialogueExpectedControl = PC->GetControlRotation();
        DialoguePlayerBefore = Player->GetActorTransform();
        Click(TEXT("DialogueTalk")); Click(TEXT("DialogueChoice2"));
        DialogueStageTime = Elapsed; Phase = 42;
    }
    else if (Phase == 42 && Elapsed > DialogueStageTime + 0.6f)
    {
        CheckRestored();
        if (DialogueNpcIndex == 0) { DialogueNpcIndex = 1; Phase = 31; }
        else { DialogueNpcIndex = 0; Phase = 43; }
    }
    else if (Phase == 43)
    {
        // A transient duplicate exercises nearest-target selection and NPC removal.
        const FTransform Transform(Npc->GetActorRotation(), Npc->GetActorLocation() + FVector(-80.f, 0.f, 0.f));
        DialogueTestNpc = GetWorld()->SpawnActorDeferred<AIslandPatrolCharacter>(AIslandPatrolCharacter::StaticClass(), Transform);
        if (!DialogueTestNpc) { Check(false, TEXT("Dialogue destruction test NPC spawned")); Phase = 12; return; }
        DialogueTestNpc->PatrolName = Npc->PatrolName;
        DialogueTestNpc->PatrolPoints = Npc->PatrolPoints;
        DialogueTestNpc->WalkAnimation = Npc->WalkAnimation; DialogueTestNpc->IdleAnimation = Npc->IdleAnimation;
        DialogueTestNpc->GetMesh()->SetSkeletalMeshAsset(Npc->GetMesh()->GetSkeletalMeshAsset());
        DialogueTestNpc->GetMesh()->SetMaterial(0, Npc->GetMesh()->GetMaterial(0));
        DialogueTestNpc->FinishSpawning(Transform);
        MovePlayer(Npc->GetActorLocation() + FVector(-160.f, -120.f, 0.f));
        PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
        Controls->SetVisibility(ESlateVisibility::Hidden); Controls->SetIsEnabled(false);
        DialogueStageTime = Elapsed; Phase = 44;
    }
    else if (Phase == 44 && Elapsed > DialogueStageTime + 0.5f)
    {
        Check(DialogueManager->GetCandidate() == DialogueTestNpc, TEXT("Dialogue chooses closest of multiple nearby NPCs"));
        DialoguePreviousView = PC->GetViewTarget(); DialogueExpectedControl = PC->GetControlRotation();
        Click(TEXT("DialogueTalk"));
        DialogueStageTime = Elapsed; Phase = 45;
    }
    else if (Phase == 45 && Elapsed > DialogueStageTime + 0.6f)
    {
        Check(DialogueManager->IsConversationActive(), TEXT("Dialogue removal test session active"));
        DialogueTestNpc->Destroy();
        DialogueStageTime = Elapsed; Phase = 46;
    }
    else if (Phase == 46 && Elapsed > DialogueStageTime + 0.6f)
    {
        Check(!DialogueManager->IsConversationActive() && Player->InputEnabled() && Player->GetCharacterMovement()->MovementMode == MOVE_Walking,
            TEXT("NPC removal closes dialogue and restores pawn input"));
        Check(PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(), TEXT("Dialogue preserves pre-existing input lock counts"));
        Check(Controls->GetVisibility() == ESlateVisibility::Hidden && !Controls->GetIsEnabled(), TEXT("Dialogue preserves pre-existing hidden/disabled controls"));
        Check(PC->GetViewTarget() == DialoguePreviousView, TEXT("NPC removal restores original camera"));
        PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false);
        Controls->SetVisibility(static_cast<ESlateVisibility>(DialogueControlsVisibility)); Controls->SetIsEnabled(bDialogueControlsEnabled);
        MovePlayer(Npc->GetActorLocation() + FVector(-160.f, -120.f, 0.f));
        DialogueStageTime = Elapsed; Phase = 47;
    }
    else if (Phase == 47 && Elapsed > DialogueStageTime + 0.5f)
    {
        DialoguePreviousView = PC->GetViewTarget(); DialogueExpectedControl = PC->GetControlRotation();
        DialoguePlayerBefore = Player->GetActorTransform();
        Click(TEXT("DialogueTalk")); UI->RemoveFromParent();
        DialogueStageTime = Elapsed; Phase = 48;
    }
    else if (Phase == 48 && Elapsed > DialogueStageTime + 0.7f)
    {
        CheckRestored();
        UI->AddToViewport(2000);
        MovePlayer(DialoguePlayerHome); PC->SetControlRotation(DialogueControlHome);
        DialogueStageTime = Elapsed; Phase = 49;
    }
    else if (Phase == 49 && Elapsed > DialogueStageTime + 0.6f)
    {
        Press(TEXT("CharacterRun"));
        Check(Player->GetCharacterMovement()->MaxWalkSpeed > BaseSpeed * 1.5f, TEXT("Run button works after dialogue"));
        Press(TEXT("CharacterRun"));
        Press(TEXT("WeatherToggle")); Check(Weather->GetWeather() == ESimpleWeather::Rain, TEXT("Weather button works after dialogue"));
        CheckWeatherSystem(1.f);
        Press(TEXT("WeatherToggle"));
        JumpStartZ = Player->GetActorLocation().Z;
        Press(TEXT("CharacterJump"));
        DialogueStageTime = Elapsed; Phase = 50;
    }
    else if (Phase == 50 && Elapsed > DialogueStageTime + 0.25f)
    {
        Check(Player->GetCharacterMovement()->IsFalling() && Player->GetActorLocation().Z > JumpStartZ + 10.f, TEXT("Jump works after dialogue"));
        Press(TEXT("CharacterJump"), true);
        DialogueStageTime = Elapsed; Phase = 51;
    }
    else if (Phase == 51 && Elapsed > DialogueStageTime + 1.7f)
    {
        Check(!Player->GetCharacterMovement()->IsFalling(), TEXT("Player lands after dialogue regression jump"));
        Shot(TEXT("dialogue_end_restored"));
        UE_LOG(LogTemp, Display, TEXT("DIALOGUE_VALIDATION_COMPLETED: dialogue branches and cleanup exercised"));
        // Keep observer input from moving the player back into a patrol's attention radius.
        PC->SetIgnoreMoveInput(true);
        bValidationHoldingInput = true;
        Phase = 12;
    }
}
