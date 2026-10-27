#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Layout/Visibility.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DialogueManagerComponent.generated.h"

class ACameraActor;
class ACharacter;
class AIslandPatrolCharacter;
class APlayerController;
class UDialogueDataAsset;
class UDialogueWidget;
class UUserWidget;
class SWidget;
struct FDialogueNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDialogueChoiceEvent, FName, NpcId, FName, ChoiceId, FName, EventTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueNodeEvent, FName, NpcId, FName, NodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueEndedEvent, FName, NpcId);

/** Local-player conversation session; the NPC remains the authority for proximity attention. */
UCLASS(ClassGroup=(Dialogue), meta=(BlueprintSpawnableComponent))
class SIMPLEWEATHER_API UDialogueManagerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDialogueManagerComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick Type, FActorComponentTickFunction* Tick) override;
    UFUNCTION(BlueprintCallable, Category="Dialogue") bool StartConversation();
    UFUNCTION(BlueprintCallable, Category="Dialogue") void Choose(int32 ChoiceIndex);
    UFUNCTION(BlueprintCallable, Category="Dialogue") void EndConversation(bool bImmediate = false);
    UFUNCTION(BlueprintPure, Category="Dialogue") bool IsConversationActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="Dialogue") AIslandPatrolCharacter* GetCandidate() const { return Candidate.Get(); }
    UFUNCTION(BlueprintPure, Category="Dialogue") FName GetCurrentNodeId() const { return CurrentNode; }
    UDialogueWidget* GetDialogueWidget() const { return Widget; }
    UDialogueDataAsset* ResolveData(AIslandPatrolCharacter* Npc) const;
    UPROPERTY(BlueprintAssignable, Category="Dialogue") FDialogueChoiceEvent OnChoiceSelected;
    UPROPERTY(BlueprintAssignable, Category="Dialogue") FDialogueNodeEvent OnNodeChanged;
    UPROPERTY(BlueprintAssignable, Category="Dialogue") FDialogueEndedEvent OnConversationEnded;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void UpdateCandidate();
    bool CanSeeNpc(AIslandPatrolCharacter* Npc) const;
    void StagePlayer(AIslandPatrolCharacter* Npc);
    void PresentNode();
    void SetOtherControlsHidden(bool bHide);
    void RestoreCamera();
    void OnMapLeaving(const FString& MapName);
    void OnApplicationBackgrounded();
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UDialogueDataAsset>> Catalog;
    UPROPERTY(Transient) TObjectPtr<UDialogueWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> ConversationCamera;
    UPROPERTY(Transient) TObjectPtr<UDialogueDataAsset> ActiveData;
    TWeakObjectPtr<AIslandPatrolCharacter> Candidate;
    TWeakObjectPtr<AIslandPatrolCharacter> ActiveNpc;
    TWeakObjectPtr<ACharacter> Player;
    TWeakObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<AActor> PreviousViewTarget;
    FRotator PreviousControlRotation = FRotator::ZeroRotator;
    FTransform PreviousPlayerTransform;
    bool bPlayerWasStaged = false;
    EMovementMode PreviousMovementMode = MOVE_Walking;
    uint8 PreviousCustomMovementMode = 0;
    bool bPreviousPawnInputEnabled = false;
    bool bAddedInputLocks = false;
    bool bPreviousCursor = false;
    TWeakPtr<SWidget> PreviousFocus;
    bool bActive = false;
    float CameraReturnRemaining = 0.f;
    FName CurrentNode;
    FDelegateHandle MapLeavingHandle;
    FDelegateHandle BackgroundHandle;
    struct FHiddenWidget { TWeakObjectPtr<UUserWidget> Widget; ESlateVisibility Visibility; bool bEnabled; };
    TArray<FHiddenWidget> HiddenWidgets;
    struct FHiddenJoystick { TWeakPtr<SWidget> Widget; EVisibility Visibility; };
    TArray<FHiddenJoystick> HiddenJoysticks;
};
