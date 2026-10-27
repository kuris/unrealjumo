#include "IslandPatrolCharacter.h"
#include "DialogueComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AIslandPatrolCharacter::AIslandPatrolCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Dialogue = CreateDefaultSubobject<UDialogueComponent>(TEXT("Dialogue"));
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI = EAutoPossessAI::Disabled;
    bUseControllerRotationYaw = false;
    GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    // These ambient patrols need no player or AI controller or runtime navmesh.
    Movement->bRunPhysicsWithNoController = true;
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 240.f, 0.f);
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxStepHeight = 45.f;
    Movement->SetWalkableFloorAngle(42.f);
    Movement->BrakingDecelerationWalking = 800.f;
}

void AIslandPatrolCharacter::BeginPlay()
{
    Super::BeginPlay();
    SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    LastLogLocation = GetActorLocation();
    if (PatrolPoints.Num() < 3 || !WalkAnimation || !IdleAnimation)
    {
        UE_LOG(LogTemp, Error, TEXT("ISLAND_PATROL_INVALID: %s needs a route and walk/idle clips"), *PatrolName);
        SetActorTickEnabled(false);
        return;
    }
    UpdateAnimation();
    UE_LOG(LogTemp, Display, TEXT("ISLAND_PATROL_START: %s points=%d speed=%.0f location=%s"),
        *PatrolName, PatrolPoints.Num(), WalkSpeed, *GetActorLocation().ToCompactString());
}

void AIslandPatrolCharacter::UpdateAnimation()
{
    const bool bWalking = !bFacingPlayer && GetVelocity().SizeSquared2D() > FMath::Square(10.f);
    if (bWalking != bPlayingWalk || GetMesh()->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
    {
        GetMesh()->PlayAnimation(bWalking ? WalkAnimation.Get() : IdleAnimation.Get(), true);
        bPlayingWalk = bWalking;
    }
}

bool AIslandPatrolCharacter::UpdatePlayerAttention(float DeltaSeconds)
{
    const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    const FVector Offset = IsValid(Player) ? Player->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
    const float EnterRadius = FMath::Max(0.f, ApproachDistance);
    const float ExitRadius = FMath::Max(EnterRadius + 1.f, ResumeDistance);
    const float Radius = bFacingPlayer ? ExitRadius : EnterRadius;
    const bool bShouldFace = IsValid(Player) && Player != this
        && ((Dialogue && Dialogue->IsInConversation())
            || (FMath::Abs(Offset.Z) <= FMath::Max(0.f, MaxInteractionHeight)
                && Offset.SizeSquared2D() <= FMath::Square(Radius)));
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (bShouldFace != bFacingPlayer)
    {
        bFacingPlayer = bShouldFace;
        Movement->bOrientRotationToMovement = !bFacingPlayer;
        StuckTime = 0.f;
        bReportedStuck = false;
        UE_LOG(LogTemp, Display, TEXT("ISLAND_PATROL_ATTENTION: %s %s distance=%.0f target=%d"),
            *PatrolName, bFacingPlayer ? TEXT("face player") : TEXT("resume patrol"), Offset.Size2D(), TargetIndex);
    }
    if (!bFacingPlayer) return false;

    // Clear queued walking input as well as velocity; leave gravity and collisions active.
    ConsumeMovementInputVector();
    const float VerticalVelocity = Movement->Velocity.Z;
    Movement->StopMovementImmediately();
    if (Movement->IsFalling()) Movement->Velocity.Z = VerticalVelocity;
    if (!Offset.IsNearlyZero(1.f) && Offset.SizeSquared2D() > 1.f)
    {
        const float Yaw = FMath::FixedTurn(GetActorRotation().Yaw, Offset.Rotation().Yaw,
            FMath::Max(0.f, FacePlayerTurnSpeed) * DeltaSeconds);
        SetActorRotation(FRotator(0.f, Yaw, 0.f));
    }
    UpdateAnimation();
    return true;
}

void AIslandPatrolCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (PatrolPoints.Num() < 3) return;
    if (UpdatePlayerAttention(DeltaSeconds)) return;
    FVector ToTarget = PatrolPoints[TargetIndex] - GetActorLocation();
    ToTarget.Z = 0.f;
    if (ToTarget.SizeSquared() < FMath::Square(50.f))
    {
        TargetIndex = (TargetIndex + 1) % PatrolPoints.Num();
        ++ReachedPoints;
        StuckTime = 0.f;
        bReportedStuck = false;
        ToTarget = PatrolPoints[TargetIndex] - GetActorLocation();
        ToTarget.Z = 0.f;
    }
    AddMovementInput(ToTarget.GetSafeNormal(), 1.f, true);
    UpdateAnimation();
    StuckTime = GetVelocity().SizeSquared2D() < FMath::Square(10.f) ? StuckTime + DeltaSeconds : 0.f;
    if (StuckTime > 5.f && !bReportedStuck)
    {
        bReportedStuck = true;
        UE_LOG(LogTemp, Warning, TEXT("ISLAND_PATROL_BLOCKED: %s target=%d location=%s"),
            *PatrolName, TargetIndex, *GetActorLocation().ToCompactString());
    }
    LogTime += DeltaSeconds;
    if (LogTime >= 10.f)
    {
        UE_LOG(LogTemp, Display, TEXT("ISLAND_PATROL_PROGRESS: %s reached=%d target=%d speed=%.0f moved=%.0f location=%s"),
            *PatrolName, ReachedPoints, TargetIndex, GetVelocity().Size2D(),
            FVector::Dist2D(LastLogLocation, GetActorLocation()), *GetActorLocation().ToCompactString());
        LastLogLocation = GetActorLocation();
        LogTime = 0.f;
    }
}
