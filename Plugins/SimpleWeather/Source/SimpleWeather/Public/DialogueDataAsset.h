#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueDataAsset.generated.h"

USTRUCT(BlueprintType)
struct SIMPLEWEATHER_API FDialogueChoice
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName NextNode;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") bool bEndConversation = false;
    /** Optional integration key for quests, affinity or rewards; no side effects by default. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName EventTag;
};

USTRUCT(BlueprintType)
struct SIMPLEWEATHER_API FDialogueNode
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TArray<FDialogueChoice> Choices;
};

USTRUCT(BlueprintType)
struct SIMPLEWEATHER_API FDialoguePresentation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float PlayerDistanceCm = 200.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float CameraDistanceCm = 340.f;
    /** Negative places the player on the left and NPC on the right of the conversation frame. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float CameraSideOffsetCm = -260.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float CameraHeightCm = 110.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float FocusHeightCm = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") float FieldOfView = 60.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") bool bStagePlayer = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Presentation") bool bRestorePlayerTransform = true;
};

/** Can be authored as a Data Asset, or populated from the packaged JSON catalog. */
UCLASS(BlueprintType)
class SIMPLEWEATHER_API UDialogueDataAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName NpcId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName EntryNode;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TArray<FDialogueNode> Nodes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FDialoguePresentation Presentation;
    const FDialogueNode* FindNode(FName Id) const;
    bool Validate(FString& Error) const;
    static bool LoadCatalog(UObject* Outer, TMap<FName, TObjectPtr<UDialogueDataAsset>>& Out, FString& Error);
};
