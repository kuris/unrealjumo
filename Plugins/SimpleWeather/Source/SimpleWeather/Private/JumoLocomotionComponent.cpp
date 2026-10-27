#include "JumoLocomotionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

UJumoLocomotionComponent::UJumoLocomotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAsset(TEXT("/Game/Characters/JumoAnimations/jumo_final_glb_riggedidle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAsset(TEXT("/Game/Characters/JumoAnimations/jumo_final_glb_riggedwalk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAsset(TEXT("/Game/Characters/JumoAnimations/jumo_final_glb_riggedrun"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> JumpAsset(TEXT("/Game/Characters/JumoAnimations/jumo_final_glb_riggedjump"));
    Idle = IdleAsset.Object; Walk = WalkAsset.Object; Run = RunAsset.Object; Jump = JumpAsset.Object;
}

bool UJumoLocomotionComponent::IsCompatible() const
{
    const ACharacter* Owner = Cast<ACharacter>(GetOwner());
    const USkeletalMesh* Mesh = Owner && Owner->GetMesh() ? Owner->GetMesh()->GetSkeletalMeshAsset() : nullptr;
    return Mesh && Idle && Walk && Run && Jump && Mesh->GetSkeleton() == Idle->GetSkeleton();
}

void UJumoLocomotionComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (!IsCompatible())
    {
        UE_LOG(LogTemp, Warning, TEXT("JUMO_LOCOMOTION_SKIPPED: incompatible pawn or missing clips"));
        SetComponentTickEnabled(false);
        return;
    }
    OriginalAnimClass = Character->GetMesh()->GetAnimClass();
    OriginalWalkingSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
    UE_LOG(LogTemp, Display, TEXT("JUMO_LOCOMOTION_READY: idle/walk/run/jump, base speed %.0f"), OriginalWalkingSpeed);
}

void UJumoLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Character.IsValid() || !IsCompatible()) return;
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    const float Speed = Character->GetVelocity().Size2D();
    const bool bAirborne = Movement->IsFalling();
    const bool bRunning = Movement->MaxWalkSpeed > OriginalWalkingSpeed * 1.1f;
    const FString NewState = bAirborne ? TEXT("Jump") : Speed < 10.f ? TEXT("Idle") : bRunning ? TEXT("Run") : TEXT("Walk");
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (NewState != CurrentState)
    {
        UAnimSequence* Clip = bAirborne ? Jump.Get() : Speed < 10.f ? Idle.Get() : bRunning ? Run.Get() : Walk.Get();
        Mesh->PlayAnimation(Clip, !bAirborne);
        if (bAirborne)
        {
            const float AirTime = FMath::Max(0.4f, 2.f * Movement->JumpZVelocity / FMath::Max(1.f, FMath::Abs(Movement->GetGravityZ())));
            Mesh->GetSingleNodeInstance()->SetPlayRate(Clip->GetPlayLength() / AirTime);
            // Stepping off a ledge uses the airborne part instead of takeoff.
            if (Movement->Velocity.Z <= 0.f) Mesh->GetSingleNodeInstance()->SetPosition(Clip->GetPlayLength() * 0.45f, false);
        }
        else Mesh->GetSingleNodeInstance()->SetPlayRate(1.f);
        CurrentState = NewState;
        UE_LOG(LogTemp, Display, TEXT("JUMO_LOCOMOTION_STATE: %s clip=%s speed=%.0f"), *CurrentState, *Clip->GetName(), Speed);
    }
    if (!bAirborne && Speed >= 10.f && Mesh->GetSingleNodeInstance())
    {
        const float ReferenceSpeed = OriginalWalkingSpeed * (bRunning ? 1.6f : 1.f);
        Mesh->GetSingleNodeInstance()->SetPlayRate(FMath::Clamp(Speed / FMath::Max(1.f, ReferenceSpeed), 0.65f, 1.6f));
    }
}

void UJumoLocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Character.IsValid() && OriginalAnimClass && EndPlayReason != EEndPlayReason::Destroyed)
        Character->GetMesh()->SetAnimInstanceClass(OriginalAnimClass);
    Super::EndPlay(EndPlayReason);
}
