#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IslandPatrolCharacter.generated.h"

class UAnimSequence;
class UDialogueComponent;

/** A ground-following walking patrol for the island's two background characters. */
UCLASS()
class SIMPLEWEATHER_API AIslandPatrolCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AIslandPatrolCharacter();
    virtual void Tick(float DeltaSeconds) override;
    int32 GetReachedPatrolPoints() const { return ReachedPoints; }

    UFUNCTION(BlueprintPure, Category="Island Patrol|Conversation")
    bool IsFacingPlayer() const { return bFacingPlayer; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island Patrol|Conversation")
    TObjectPtr<UDialogueComponent> Dialogue;

    /** Stop walking when the player is within this horizontal distance (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol|Conversation", meta=(ClampMin="0", Units="cm"))
    float ApproachDistance = 250.f;

    /** Resume beyond this distance; clamped above ApproachDistance at runtime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol|Conversation", meta=(ClampMin="0", Units="cm"))
    float ResumeDistance = 350.f;

    /** Avoid reacting to a player on a different elevation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol|Conversation", meta=(ClampMin="0", Units="cm"))
    float MaxInteractionHeight = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol|Conversation", meta=(ClampMin="0", Units="deg/s"))
    float FacePlayerTurnSpeed = 240.f;

    /** Ordered world-space ground positions sampled from the island landscape. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol")
    TArray<FVector> PatrolPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol")
    TObjectPtr<UAnimSequence> WalkAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol")
    TObjectPtr<UAnimSequence> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol")
    float WalkSpeed = 160.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island Patrol")
    FString PatrolName;

protected:
    virtual void BeginPlay() override;

private:
    int32 TargetIndex = 1;
    int32 ReachedPoints = 0;
    float LogTime = 0.f;
    float StuckTime = 0.f;
    FVector LastLogLocation = FVector::ZeroVector;
    bool bPlayingWalk = false;
    bool bReportedStuck = false;
    bool bFacingPlayer = false;
    bool UpdatePlayerAttention(float DeltaSeconds);
    void UpdateAnimation();
};
