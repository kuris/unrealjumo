#include "DialogueWidget.h"
#include "DialogueDataAsset.h"
#include "DialogueManagerComponent.h"
#include "IslandPatrolCharacter.h"
#include "IslandUIStyle.h"
#include "WeatherControlWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectIterator.h"

namespace
{
const FLinearColor Ink(0.045f, 0.035f, 0.025f, 1.f);
const FLinearColor PaperColor(0.91f, 0.83f, 0.67f, 0.99f);
const FLinearColor Gold(0.70f, 0.43f, 0.11f, 1.f);
const FLinearColor LightGold(0.96f, 0.75f, 0.31f, 1.f);
FButtonStyle RoundedButton(bool bGold, bool bDark = false)
{
    const FLinearColor Normal = bDark ? Ink : bGold ? Gold : FLinearColor(0.96f, 0.90f, 0.78f, 1.f);
    const FLinearColor Outline = bDark ? FLinearColor(0.94f, 0.88f, 0.74f) : bGold ? LightGold : FLinearColor(0.60f, 0.48f, 0.29f);
    FButtonStyle Style;
    Style.SetNormal(FSlateRoundedBoxBrush(Normal, 14.f, Outline, 1.5f));
    Style.SetHovered(FSlateRoundedBoxBrush(bDark ? Ink * 1.4f : bGold ? LightGold : FLinearColor(1.f, 0.96f, 0.86f), 14.f, Outline, 2.f));
    Style.SetPressed(FSlateRoundedBoxBrush(bDark ? Ink : bGold ? FLinearColor(0.48f, 0.27f, 0.06f) : PaperColor, 14.f, Outline, 2.f));
    Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.4f, 0.35f, 0.25f, 0.6f), 14.f));
    Style.SetNormalPadding(FMargin(14.f, 5.f));
    Style.SetPressedPadding(FMargin(14.f, 6.f, 14.f, 4.f));
    return Style;
}
}

void UDialogueWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>();
    Safe->SetSidesToPad(true, true, true, true);
    Safe->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Safe;
    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Safe->SetContent(Canvas);

    auto Text = [&](FName Id, const FText& Value, int32 Size, FLinearColor Color)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Id);
        Label->SetText(Value);
        Label->SetFont(IslandUIStyle::Font(Size));
        Label->SetColorAndOpacity(FSlateColor(Color));
        Label->SetJustification(Id == TEXT("DialogueSpeech") || Id == TEXT("DialogueSpeaker") ? ETextJustify::Left : ETextJustify::Center);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        return Label;
    };
    auto Button = [&](FName Id, bool bGold, bool bDark = false)
    {
        UButton* Result = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Id);
        Result->SetStyle(RoundedButton(bGold, bDark));
        Result->SetTouchMethod(EButtonTouchMethod::DownAndUp);
        Result->SetClickMethod(EButtonClickMethod::DownAndUp);
        return Result;
    };

    ModalBlocker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueInputShield"));
    ModalBlocker->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.025f));
    UCanvasPanelSlot* ModalSlot = Canvas->AddChildToCanvas(ModalBlocker);
    ModalSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    ModalSlot->SetOffsets(FMargin(0.f));
    ModalSlot->SetZOrder(1);
    ModalBlocker->SetVisibility(ESlateVisibility::Collapsed);

    TalkButton = Button(TEXT("DialogueTalk"), true);
    TalkButton->SetContent(Text(TEXT("DialogueTalkLabel"), NSLOCTEXT("Dialogue", "Talk", "···  대화"), 24, Ink));
    TalkButton->OnClicked.AddDynamic(this, &UDialogueWidget::Talk);
    UCanvasPanelSlot* TalkSlot = Canvas->AddChildToCanvas(TalkButton);
    TalkSlot->SetAlignment(FVector2D(1.f, 0.5f));
    TalkSlot->SetZOrder(3);
    TalkSlot->SetSize(FVector2D(180.f, 64.f));
    TalkButton->SetVisibility(ESlateVisibility::Collapsed);

    Bubble = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueNpcBubble"));
    Bubble->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.98f, 0.94f, 0.84f), 14.f, Ink, 1.5f));
    UTextBlock* Dots = Text(TEXT("DialogueBubbleDots"), FText::FromString(TEXT("···")), 24, Ink);
    Dots->SetJustification(ETextJustify::Center);
    Bubble->SetContent(Dots);
    UCanvasPanelSlot* BubbleSlot = Canvas->AddChildToCanvas(Bubble);
    BubbleSlot->SetAlignment(FVector2D(0.5f, 1.f));
    BubbleSlot->SetSize(FVector2D(50.f, 40.f));
    Bubble->SetVisibility(ESlateVisibility::Collapsed);

    PanelShadow = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueShadow"));
    PanelShadow->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.23f), 24.f));
    UCanvasPanelSlot* ShadowSlot = Canvas->AddChildToCanvas(PanelShadow);
    ShadowSlot->SetAnchors(FAnchors(0.5f, 1.f));
    ShadowSlot->SetAlignment(FVector2D(0.5f, 1.f));
    ShadowSlot->SetZOrder(2);
    PanelShadow->SetVisibility(ESlateVisibility::Collapsed);
    Paper = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialoguePaper"));
    Paper->SetBrush(FSlateRoundedBoxBrush(PaperColor, 22.f, FLinearColor(0.57f, 0.44f, 0.26f), 1.f));
    Paper->SetPadding(FMargin(22.f, 12.f, 22.f, 16.f));
    UCanvasPanelSlot* PaperSlot = Canvas->AddChildToCanvas(Paper);
    PaperSlot->SetAnchors(FAnchors(0.5f, 1.f));
    PaperSlot->SetAlignment(FVector2D(0.5f, 1.f));
    PaperSlot->SetZOrder(3);
    Paper->SetVisibility(ESlateVisibility::Collapsed);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Paper->SetContent(Content);
    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    UBorder* InkName = WidgetTree->ConstructWidget<UBorder>();
    InkName->SetBrush(FSlateRoundedBoxBrush(Ink, FVector4(5.f, 15.f, 7.f, 12.f)));
    InkName->SetPadding(FMargin(20.f, 3.f));
    Speaker = Text(TEXT("DialogueSpeaker"), FText::GetEmpty(), 24, FLinearColor(0.98f, 0.94f, 0.84f));
    InkName->SetContent(Speaker);
    Header->AddChildToHorizontalBox(InkName)->SetVerticalAlignment(VAlign_Center);
    USizeBox* HeaderSpace = WidgetTree->ConstructWidget<USizeBox>();
    Header->AddChildToHorizontalBox(HeaderSpace)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CloseButton = Button(TEXT("DialogueClose"), false, true);
    CloseButton->SetContent(Text(TEXT("DialogueCloseLabel"), FText::FromString(TEXT("×")), 26, FLinearColor::White));
    CloseButton->OnClicked.AddDynamic(this, &UDialogueWidget::Close);
    USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>();
    CloseSize->SetWidthOverride(48.f); CloseSize->SetHeightOverride(44.f); CloseSize->SetContent(CloseButton);
    Header->AddChildToHorizontalBox(CloseSize);

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Body);
    Speech = Text(TEXT("DialogueSpeech"), FText::GetEmpty(), 21, Ink);
    Speech->SetAutoWrapText(true);
    Speech->SetLineHeightPercentage(1.2f);
    Body->AddChildToVerticalBox(Speech)->SetPadding(FMargin(2.f, 0.f, 2.f, 8.f));
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UButton* Choice = Button(FName(*FString::Printf(TEXT("DialogueChoice%d"), Index)), Index == 0);
        UTextBlock* Label = Text(FName(*FString::Printf(TEXT("DialogueChoiceLabel%d"), Index)), FText::GetEmpty(), 19,
            Index == 0 ? FLinearColor(1.f, 0.97f, 0.88f) : Ink);
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(true);
        Choice->SetContent(Label);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Label->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
        }
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetMinDesiredHeight(48.f);
        Size->SetContent(Choice);
        Body->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.f, 3.f));
        ChoiceButtons.Add(Choice); ChoiceLabels.Add(Label);
    }
    ChoiceButtons[0]->OnClicked.AddDynamic(this, &UDialogueWidget::Choice0);
    ChoiceButtons[1]->OnClicked.AddDynamic(this, &UDialogueWidget::Choice1);
    ChoiceButtons[2]->OnClicked.AddDynamic(this, &UDialogueWidget::Choice2);
    ResponsiveSlots = { PaperSlot, TalkSlot, BubbleSlot, ShadowSlot };
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    for (TObjectIterator<UWeatherControlWidget> It; It; ++It)
        if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == GetOwningPlayer()) ControlsForDiagnostics = *It;
#endif
}

void UDialogueWidget::SetCandidate(AIslandPatrolCharacter* Npc)
{
    Candidate = Npc;
    const bool bShow = Npc && (!Manager.IsValid() || !Manager->IsConversationActive());
    TalkButton->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    Bubble->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
void UDialogueWidget::ShowNode(const FText& Name, const FDialogueNode& Node)
{
    Speaker->SetText(Name); Speech->SetText(Node.Text);
    // Supply wrapping widths explicitly: centered button slots must not retain a previous node's narrow desired size.
    const float AvailableWidth = FMath::Max(180.f, float(ResponsiveSlots[0]->GetSize().X - 44.f));
    Speech->SetWrapTextAt(AvailableWidth - 4.f);
    for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
    {
        ChoiceButtons[Index]->GetParent()->SetVisibility(Node.Choices.IsValidIndex(Index) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (Node.Choices.IsValidIndex(Index))
        {
            ChoiceLabels[Index]->SetWrapTextAt(AvailableWidth - 28.f);
            ChoiceLabels[Index]->SetText(Node.Choices[Index].Text);
        }
    }
    Paper->SetVisibility(ESlateVisibility::Visible);
    PanelShadow->SetVisibility(ESlateVisibility::HitTestInvisible);
    ModalBlocker->SetVisibility(ESlateVisibility::Visible);
    TalkButton->SetVisibility(ESlateVisibility::Collapsed);
    Bubble->SetVisibility(ESlateVisibility::Collapsed);
}
void UDialogueWidget::HideConversation()
{
    Paper->SetVisibility(ESlateVisibility::Collapsed);
    PanelShadow->SetVisibility(ESlateVisibility::Collapsed);
    ModalBlocker->SetVisibility(ESlateVisibility::Collapsed);
    SetCandidate(nullptr);
}
bool UDialogueWidget::IsPromptVisible() const { return TalkButton && TalkButton->GetVisibility() == ESlateVisibility::Visible; }
bool UDialogueWidget::IsConversationVisible() const { return Paper && Paper->GetVisibility() == ESlateVisibility::Visible; }

void UDialogueWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (!Canvas || ResponsiveSlots.Num() != 4) return;
    const FVector2D Size = Canvas->GetCachedGeometry().GetLocalSize();
    if (Size.X < 1.f || Size.Y < 1.f) return;
    const float PanelHeight = FMath::Min(FMath::Max(320.f, float(Size.Y * 0.39f)), float(Size.Y * 0.6f));
    ResponsiveSlots[0]->SetPosition(FVector2D(0.f, -16.f));
    ResponsiveSlots[0]->SetSize(FVector2D(FMath::Min(float(Size.X - 32.f), 1000.f), PanelHeight));
    ResponsiveSlots[3]->SetPosition(FVector2D(0.f, -9.f));
    ResponsiveSlots[3]->SetSize(ResponsiveSlots[0]->GetSize());
    Speech->SetWrapTextAt(FMath::Max(180.f, float(ResponsiveSlots[0]->GetSize().X - 48.f)));
    for (UTextBlock* Label : ChoiceLabels) Label->SetWrapTextAt(FMath::Max(160.f, float(ResponsiveSlots[0]->GetSize().X - 72.f)));
    const bool bShort = Size.Y < 640.f;
    ResponsiveSlots[1]->SetPosition(FVector2D(Size.X - (bShort ? 180.f : 24.f), Size.Y * (bShort ? 0.44f : 0.53f)));
    if (Candidate.IsValid() && IsPromptVisible())
    {
        FVector2D Point;
        const FBoxSphereBounds Bounds = Candidate->GetMesh()->GetBounds();
        const FVector HeadPoint(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z + 20.f);
        const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), HeadPoint, Point, false);
        if (bProjected)
        {
            Point = Canvas->GetCachedGeometry().AbsoluteToLocal(Geometry.LocalToAbsolute(Point));
            ResponsiveSlots[2]->SetPosition(Point);
        }
        Bubble->SetVisibility(bProjected && Point.X > 24.f && Point.X < Size.X - 24.f && Point.Y > 40.f && Point.Y < Size.Y
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    // Pixel rectangles let device validation inject real touch events after DPI/safe-area layout.
    int32 ViewWidth = 0, ViewHeight = 0;
    if (GetOwningPlayer()) GetOwningPlayer()->GetViewportSize(ViewWidth, ViewHeight);
    FString Rects = FString::Printf(TEXT("npc=%s node=%s viewport=%d,%d"), Candidate.IsValid() ? *Candidate->PatrolName : TEXT("none"),
        Manager.IsValid() ? *Manager->GetCurrentNodeId().ToString() : TEXT("none"), ViewWidth, ViewHeight);
    const UWeatherControlWidget* DiagnosticWeather = Cast<UWeatherControlWidget>(ControlsForDiagnostics.Get());
    UUserWidget* DiagnosticTouch = DiagnosticWeather ? DiagnosticWeather->GetLegacyTouchWidget() : nullptr;
    Rects += FString::Printf(TEXT(" touchUiVisible=%d"), DiagnosticTouch && DiagnosticTouch->IsVisible());
    auto Rect = [&](const TCHAR* Name, UWidget* Control, bool bVisible)
    {
        if (!Control || !bVisible) return;
        const FGeometry& Cached = Control->GetCachedGeometry();
        FVector2D Start, End, Unused;
        USlateBlueprintLibrary::LocalToViewport(this, Cached, FVector2D::ZeroVector, Start, Unused);
        USlateBlueprintLibrary::LocalToViewport(this, Cached, Cached.GetLocalSize(), End, Unused);
        Rects += FString::Printf(TEXT(" %s=%d,%d,%d,%d"), Name, FMath::RoundToInt(Start.X), FMath::RoundToInt(Start.Y),
            FMath::RoundToInt(End.X - Start.X), FMath::RoundToInt(End.Y - Start.Y));
    };
    Rect(TEXT("talk"), TalkButton, IsPromptVisible());
    Rect(TEXT("close"), CloseButton, IsConversationVisible());
    Rect(TEXT("paper"), Paper, IsConversationVisible());
    Rect(TEXT("speech"), Speech, IsConversationVisible());
    Rect(TEXT("speaker"), Speaker, IsConversationVisible());
    for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
        Rect(*FString::Printf(TEXT("choice%d"), Index), ChoiceButtons[Index], IsConversationVisible()
            && ChoiceButtons[Index]->GetParent()->GetVisibility() != ESlateVisibility::Collapsed);
    Rect(TEXT("bubble"), Bubble, Bubble->GetVisibility() != ESlateVisibility::Collapsed);
    if (ControlsForDiagnostics.IsValid() && ControlsForDiagnostics->GetIsEnabled()
        && ControlsForDiagnostics->GetVisibility() != ESlateVisibility::Collapsed)
    {
        Rect(TEXT("run"), ControlsForDiagnostics->GetWidgetFromName(TEXT("CharacterRun")), true);
        Rect(TEXT("jump"), ControlsForDiagnostics->GetWidgetFromName(TEXT("CharacterJump")), true);
        UWidget* SwimControl = ControlsForDiagnostics->GetWidgetFromName(TEXT("CharacterSwim"));
        Rect(TEXT("swim"), SwimControl, SwimControl && SwimControl->IsVisible());
        Rect(TEXT("weather"), ControlsForDiagnostics->GetWidgetFromName(TEXT("WeatherToggle")), true);
        Rect(TEXT("slider"), ControlsForDiagnostics->GetWidgetFromName(TEXT("RainAmount")), true);
        if (const UWeatherControlWidget* WeatherUI = Cast<UWeatherControlWidget>(ControlsForDiagnostics.Get()))
        {
            if (UUserWidget* Touch = WeatherUI->GetLegacyTouchWidget())
            {
                Rect(TEXT("moveStick"), Touch->GetWidgetFromName(TEXT("Thumbstick_Move")), true);
                Rect(TEXT("lookStick"), Touch->GetWidgetFromName(TEXT("Thumbstick_Aim")), true);
                UWidget* OldJump = Touch->GetWidgetFromName(TEXT("Btn_Jump"));
                Rect(TEXT("legacyJump"), OldJump, OldJump && OldJump->GetIsEnabled() && OldJump->IsVisible());
            }
        }
    }
    if (Rects != LastLayoutLog)
    {
        LastLayoutLog = Rects;
        UE_LOG(LogTemp, Display, TEXT("DIALOGUE_UI_RECTS: %s"), *Rects);
    }
#endif
}

void UDialogueWidget::Talk() { if (Manager.IsValid()) Manager->StartConversation(); }
void UDialogueWidget::Close() { if (Manager.IsValid()) Manager->EndConversation(); }
void UDialogueWidget::Choice0() { if (Manager.IsValid()) Manager->Choose(0); }
void UDialogueWidget::Choice1() { if (Manager.IsValid()) Manager->Choose(1); }
void UDialogueWidget::Choice2() { if (Manager.IsValid()) Manager->Choose(2); }
FReply UDialogueWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Manager.IsValid() && Manager->IsConversationActive() && Event.GetKey() == EKeys::Escape)
    { Close(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}
void UDialogueWidget::NativeDestruct()
{
    if (Manager.IsValid()) Manager->EndConversation(true);
    Super::NativeDestruct();
}
