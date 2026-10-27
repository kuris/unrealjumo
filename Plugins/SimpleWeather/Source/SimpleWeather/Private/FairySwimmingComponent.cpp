#include "FairySwimmingComponent.h"
#include "JumoLocomotionComponent.h"
#include "DialogueManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"

UFairySwimmingComponent::UFairySwimmingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UFairySwimmingComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (!Character.IsValid()) { SetComponentTickEnabled(false); return; }
    Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
    RefreshWaterBodies();
}

void UFairySwimmingComponent::RefreshWaterBodies()
{
    WaterBodies.Reset();
    for (TActorIterator<AWaterBody> It(GetWorld()); It; ++It)
        if (UWaterBodyComponent* Body = It->GetWaterBodyComponent()) WaterBodies.Add(Body);
    RefreshAt = GetWorld()->GetTimeSeconds() + 2.f;
}

bool UFairySwimmingComponent::QueryWater(float& OutSurface, float& OutDepth)
{
    if (GetWorld()->GetTimeSeconds() >= RefreshAt) RefreshWaterBodies();
    const FVector Location = Character->GetActorLocation();
    bool bFound = false;
    OutSurface = -BIG_NUMBER;
    for (const auto& WeakBody : WaterBodies)
    {
        UWaterBodyComponent* Body = WeakBody.Get();
        if (!Body || !Body->Bounds.GetBox().IsInsideXY(Location)) continue;
        auto Query = Body->TryQueryWaterInfoClosestToWorldLocation(Location,
            EWaterBodyQueryFlags::ComputeLocation | EWaterBodyQueryFlags::IncludeWaves | EWaterBodyQueryFlags::SimpleWaves);
        if (!Query.HasValue() || Query.GetValue().IsInExclusionVolume()) continue;
        const float Z = Query.GetValue().GetWaterSurfaceLocation().Z;
        if (Z > OutSurface) { OutSurface = Z; bFound = true; }
    }
    if (!bFound) return false;
    // Query real collision rather than the landscape-only Water depth texture:
    // piers, rocks and walkable platforms must remain dry footing.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FairySwimGround), false, Character.Get());
    for (const auto& Body : WaterBodies) if (Body.IsValid()) Params.AddIgnoredActor(Body->GetOwner());
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Ground;
    FVector Start(Location.X, Location.Y, FMath::Max(Location.Z + 100.f, OutSurface + 250.f));
    const bool bHit = GetWorld()->LineTraceSingleByObjectType(Ground, Start,
        FVector(Location.X, Location.Y, OutSurface - 20000.f), Objects, Params);
    OutDepth = bHit ? FMath::Max(0.f, OutSurface - Ground.ImpactPoint.Z) : 20000.f;
    return true;
}

void UFairySwimmingComponent::EnterWater()
{
    ACharacter* Pawn = Character.Get();
    UCharacterMovementComponent* Move = Pawn->GetCharacterMovement();
    FActorSpawnParameters Params;
    Params.Owner = Pawn;
    Params.ObjectFlags |= RF_Transient;
    SwimmingVolume = GetWorld()->SpawnActor<APhysicsVolume>(Params);
    if (!SwimmingVolume) return;
    // The ocean renderer is not a PhysicsVolume. Assign a private, brushless
    // water volume only to this capsule so native PhysSwimming handles collision
    // and acceleration without flooding the whole island or changing the map.
    SwimmingVolume->bWaterVolume = true;
    SwimmingVolume->FluidFriction = 2.f;
    SwimmingVolume->SetActorEnableCollision(false);
    SwimmingVolume->SetActorHiddenInGame(true);
    UCapsuleComponent* Capsule = Pawn->GetCapsuleComponent();
    PreviousVolume = Capsule->GetPhysicsVolume();
    bPreviousVolumeUpdates = Capsule->GetShouldUpdatePhysicsVolume();
    PreviousBuoyancy = Move->Buoyancy;
    PreviousSwimSpeed = Move->MaxSwimSpeed;
    bSwimming = true;
    bCanStartSwimming = false;
    Capsule->SetShouldUpdatePhysicsVolume(false);
    Capsule->SetPhysicsVolume(SwimmingVolume, false);
    Move->Buoyancy = 1.f;
    Move->MaxSwimSpeed = SwimSpeed;
    Pawn->StopJumping();
    Move->Velocity.Z = 0.f;
    Move->SetMovementMode(MOVE_Swimming);
    UE_LOG(LogTemp, Display, TEXT("FAIRY_SWIM_ENTER: surface=%.1f position=%s"), SurfaceHeight, *Pawn->GetActorLocation().ToString());
}

bool UFairySwimmingComponent::StartSwimming()
{
    if (!Character.IsValid() || bSwimming) return false;
    auto* Pawn = Character.Get();
    auto* Move = Pawn->GetCharacterMovement();
    auto* Playback = Pawn->FindComponentByClass<UJumoLocomotionComponent>();
    auto* Dialogue = Pawn->FindComponentByClass<UDialogueManagerComponent>();
    float Surface = 0.f, Depth = 0.f;
    const float FeetZ = Pawn->GetActorLocation().Z - Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if (!Playback || !Playback->HasSwimmingAnimations() || (Dialogue && Dialogue->IsConversationActive())
        || !(Move->IsMovingOnGround() || Move->IsFalling()) || !QueryWater(Surface, Depth)
        || Depth < FMath::Max(ManualStartDepth, ExitDepth + 10.f) || FeetZ > Surface + 5.f) return false;
    SurfaceHeight = Surface;
    EnterWater();
    UE_LOG(LogTemp, Display, TEXT("FAIRY_SWIM_MANUAL: started=%d depth=%.1f"), bSwimming, Depth);
    return bSwimming;
}

void UFairySwimmingComponent::LeaveWater(bool bRestoreMovement)
{
    if (!bSwimming) return;
    bSwimming = false;
    if (ACharacter* Pawn = Character.Get())
    {
        auto* Move = Pawn->GetCharacterMovement();
        auto* Capsule = Pawn->GetCapsuleComponent();
        const bool bOwnVolume = Capsule->GetPhysicsVolume() == SwimmingVolume;
        if (bOwnVolume) Capsule->SetPhysicsVolume(PreviousVolume.Get(), false);
        Capsule->SetShouldUpdatePhysicsVolume(bPreviousVolumeUpdates);
        if (bPreviousVolumeUpdates) Capsule->UpdatePhysicsVolume(false);
        Move->Buoyancy = PreviousBuoyancy;
        Move->MaxSwimSpeed = PreviousSwimSpeed;
        // Never overwrite a dialogue/cutscene's MOVE_None or another system's mode.
        if (bRestoreMovement && Move->IsSwimming() && bOwnVolume) Move->SetMovementMode(MOVE_Falling);
    }
    if (SwimmingVolume) SwimmingVolume->Destroy();
    SwimmingVolume = nullptr;
    UE_LOG(LogTemp, Display, TEXT("FAIRY_SWIM_EXIT: movement restored=%d"), bRestoreMovement);
}

void UFairySwimmingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Character.IsValid()) return;
    auto* Pawn = Character.Get();
    auto* Move = Pawn->GetCharacterMovement();
    auto* Dialogue = Pawn->FindComponentByClass<UDialogueManagerComponent>();
    auto* Playback = Pawn->FindComponentByClass<UJumoLocomotionComponent>();
    const bool bCanAnimate = Playback && Playback->HasSwimmingAnimations();
    const bool bBlocked = (Dialogue && Dialogue->IsConversationActive()) || Move->MovementMode == MOVE_None || !bCanAnimate;
    bCanStartSwimming = false;
    if (bSwimming && (bBlocked || !Move->IsSwimming() || Pawn->GetCapsuleComponent()->GetPhysicsVolume() != SwimmingVolume))
    {
        LeaveWater(!bBlocked);
        return;
    }
    if (bBlocked) return;
    float Surface = 0.f, Depth = 0.f;
    const bool bWater = QueryWater(Surface, Depth);
    const float FeetZ = Pawn->GetActorLocation().Z - Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    bCanStartSwimming = !bSwimming && bWater && FeetZ <= Surface + 5.f
        && Depth >= FMath::Max(ManualStartDepth, ExitDepth + 10.f)
        && (Move->IsMovingOnGround() || Move->IsFalling());
    if (!bSwimming)
    {
        if (!bWater || Depth < EnterDepth || FeetZ > Surface + 5.f ||
            !(Move->IsMovingOnGround() || Move->IsFalling())) return;
        SurfaceHeight = Surface;
        EnterWater();
    }
    if (!bSwimming) return;
    if (!bWater || Depth <= FMath::Min(ExitDepth, EnterDepth - 10.f) || FeetZ > Surface + 180.f)
    {
        LeaveWater(true);
        return;
    }
    SurfaceHeight = Surface;
    // Treading is upright; forward swimming lies almost horizontal. Match the
    // capsule's surface offset gradually instead of pitching the collision shape.
    const float Moving = FMath::Clamp(Move->Velocity.Size2D() / 80.f, 0.f, 1.f);
    const float TargetZ = SurfaceHeight + FMath::Lerp(TreadSurfaceOffset, ForwardSurfaceOffset, Moving);
    Move->Velocity.Z = FMath::Clamp((TargetZ - Pawn->GetActorLocation().Z) * 4.f, -100.f, 100.f);
    Pawn->StopJumping();
}

void UFairySwimmingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    LeaveWater(true);
    if (Character.IsValid()) Character->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
    Super::EndPlay(EndPlayReason);
}
