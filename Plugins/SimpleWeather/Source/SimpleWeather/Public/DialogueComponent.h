#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueComponent.generated.h"
class UDialogueDataAsset;
class UDialogueManagerComponent;

UCLASS(ClassGroup=(Dialogue), meta=(BlueprintSpawnableComponent))
class SIMPLEWEATHER_API UDialogueComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** An authored asset overrides catalog lookup. Empty id falls back to PatrolName. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TObjectPtr<UDialogueDataAsset> Data;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName DialogueId;
    UFUNCTION(BlueprintPure, Category="Dialogue") bool IsInConversation() const { return ConversationOwner.IsValid(); }
    bool Acquire(UDialogueManagerComponent* Manager);
    void Release(UDialogueManagerComponent* Manager);
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TWeakObjectPtr<UDialogueManagerComponent> ConversationOwner;
};
