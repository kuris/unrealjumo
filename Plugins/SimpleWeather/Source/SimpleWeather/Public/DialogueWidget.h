#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"
class UBorder;
class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UDialogueManagerComponent;
class UTextBlock;
class UVerticalBox;
class AIslandPatrolCharacter;
struct FDialogueNode;

UCLASS()
class SIMPLEWEATHER_API UDialogueWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetManager(UDialogueManagerComponent* Value) { Manager = Value; }
    void SetCandidate(AIslandPatrolCharacter* Npc);
    void ShowNode(const FText& Name, const FDialogueNode& Node);
    void HideConversation();
    bool IsPromptVisible() const;
    bool IsConversationVisible() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UFUNCTION() void Talk();
    UFUNCTION() void Close();
    UFUNCTION() void Choice0();
    UFUNCTION() void Choice1();
    UFUNCTION() void Choice2();
    TWeakObjectPtr<UDialogueManagerComponent> Manager;
    TWeakObjectPtr<AIslandPatrolCharacter> Candidate;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TObjectPtr<UButton> TalkButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
    UPROPERTY(Transient) TObjectPtr<UBorder> Bubble;
    UPROPERTY(Transient) TObjectPtr<UBorder> ModalBlocker;
    UPROPERTY(Transient) TObjectPtr<UBorder> Paper;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Speaker;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Speech;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ChoiceButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ChoiceLabels;
    UPROPERTY(Transient) TArray<TObjectPtr<UCanvasPanelSlot>> ResponsiveSlots;
    UPROPERTY(Transient) TObjectPtr<UBorder> PanelShadow;
    FString LastLayoutLog;
    TWeakObjectPtr<UUserWidget> ControlsForDiagnostics;
};
