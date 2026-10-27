#include "DialogueDataAsset.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FDialogueNode* UDialogueDataAsset::FindNode(FName Id) const
{
    return Nodes.FindByPredicate([Id](const FDialogueNode& Node) { return Node.Id == Id; });
}

bool UDialogueDataAsset::Validate(FString& Error) const
{
    auto Fail = [&](const FString& Why) { Error = NpcId.ToString() + TEXT(": ") + Why; return false; };
    if (NpcId.IsNone() || DisplayName.IsEmpty() || !FindNode(EntryNode)) return Fail(TEXT("missing NPC identity or entry node"));
    if (!FMath::IsFinite(Presentation.PlayerDistanceCm) || Presentation.PlayerDistanceCm < 140.f || Presentation.PlayerDistanceCm > 240.f
        || !FMath::IsFinite(Presentation.CameraDistanceCm) || Presentation.CameraDistanceCm < 200.f
        || !FMath::IsFinite(Presentation.CameraSideOffsetCm) || !FMath::IsFinite(Presentation.CameraHeightCm)
        || !FMath::IsFinite(Presentation.FocusHeightCm) || !FMath::IsFinite(Presentation.FieldOfView)
        || Presentation.FieldOfView < 40.f || Presentation.FieldOfView > 90.f) return Fail(TEXT("invalid conversation presentation profile"));
    TSet<FName> Ids;
    for (const FDialogueNode& Node : Nodes)
    {
        if (Node.Id.IsNone() || Ids.Contains(Node.Id) || Node.Text.IsEmpty()) return Fail(TEXT("invalid/duplicate node"));
        Ids.Add(Node.Id);
        if (Node.Choices.IsEmpty() || Node.Choices.Num() > 3) return Fail(TEXT("nodes require 1..3 choices"));
        TSet<FName> ChoiceIds;
        for (const FDialogueChoice& Choice : Node.Choices)
        {
            if (Choice.Id.IsNone() || ChoiceIds.Contains(Choice.Id) || Choice.Text.IsEmpty()) return Fail(TEXT("invalid/duplicate choice"));
            ChoiceIds.Add(Choice.Id);
            if (!Choice.bEndConversation && !FindNode(Choice.NextNode)) return Fail(TEXT("dangling choice target"));
        }
    }
    return true;
}

bool UDialogueDataAsset::LoadCatalog(UObject* Outer, TMap<FName, TObjectPtr<UDialogueDataAsset>>& Out, FString& Error)
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SimpleWeather"));
    FString Source;
    if (!Plugin || !FFileHelper::LoadFileToString(Source, *(Plugin->GetBaseDir() / TEXT("Data/Dialogues.json"))))
    { Error = TEXT("packaged Data/Dialogues.json could not be read"); return false; }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source), Root) || !Root.IsValid())
    { Error = TEXT("invalid dialogue JSON"); return false; }
    const TArray<TSharedPtr<FJsonValue>>* Npcs = nullptr;
    if (!Root->TryGetArrayField(TEXT("npcs"), Npcs)) { Error = TEXT("missing npcs array"); return false; }
    TMap<FName, TObjectPtr<UDialogueDataAsset>> Catalog;
    auto ReadPresentation = [](const TSharedPtr<FJsonObject>& Object, FDialoguePresentation& Profile)
    {
        const TSharedPtr<FJsonObject>* Options = nullptr;
        if (!Object->TryGetObjectField(TEXT("presentation"), Options)) return;
        auto Scalar = [&](const TCHAR* Key, float& Destination)
        {
            double Value = 0;
            if ((*Options)->TryGetNumberField(Key, Value)) Destination = float(Value);
        };
        Scalar(TEXT("playerDistanceCm"), Profile.PlayerDistanceCm);
        Scalar(TEXT("cameraDistanceCm"), Profile.CameraDistanceCm);
        Scalar(TEXT("cameraSideOffsetCm"), Profile.CameraSideOffsetCm);
        Scalar(TEXT("cameraHeightCm"), Profile.CameraHeightCm);
        Scalar(TEXT("focusHeightCm"), Profile.FocusHeightCm);
        Scalar(TEXT("fieldOfView"), Profile.FieldOfView);
        (*Options)->TryGetBoolField(TEXT("stagePlayer"), Profile.bStagePlayer);
        (*Options)->TryGetBoolField(TEXT("restorePlayerTransform"), Profile.bRestorePlayerTransform);
    };
    FDialoguePresentation Defaults;
    ReadPresentation(Root, Defaults);
    for (const TSharedPtr<FJsonValue>& Value : *Npcs)
    {
        const TSharedPtr<FJsonObject> Npc = Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        FString Id, Name, Entry;
        const TArray<TSharedPtr<FJsonValue>>* NodesJson = nullptr;
        if (!Npc || !Npc->TryGetStringField(TEXT("id"), Id) || !Npc->TryGetStringField(TEXT("name"), Name)
            || !Npc->TryGetStringField(TEXT("entry"), Entry) || !Npc->TryGetArrayField(TEXT("nodes"), NodesJson))
        { Error = TEXT("invalid NPC fields"); return false; }
        UDialogueDataAsset* Data = NewObject<UDialogueDataAsset>(Outer);
        Data->Presentation = Defaults;
        ReadPresentation(Npc, Data->Presentation);
        Data->NpcId = FName(*Id); Data->DisplayName = FText::FromString(Name); Data->EntryNode = FName(*Entry);
        for (const TSharedPtr<FJsonValue>& NodeValue : *NodesJson)
        {
            const TSharedPtr<FJsonObject> Object = NodeValue->Type == EJson::Object ? NodeValue->AsObject() : nullptr;
            FString NodeId, Text;
            const TArray<TSharedPtr<FJsonValue>>* ChoicesJson = nullptr;
            if (!Object || !Object->TryGetStringField(TEXT("id"), NodeId) || !Object->TryGetStringField(TEXT("text"), Text)
                || !Object->TryGetArrayField(TEXT("choices"), ChoicesJson))
            { Error = TEXT("invalid node fields"); return false; }
            FDialogueNode Node; Node.Id = FName(*NodeId); Node.Text = FText::FromString(Text);
            for (const TSharedPtr<FJsonValue>& ChoiceValue : *ChoicesJson)
            {
                const TSharedPtr<FJsonObject> ObjectChoice = ChoiceValue->Type == EJson::Object ? ChoiceValue->AsObject() : nullptr;
                FString ChoiceId, ChoiceText, Next, Event;
                if (!ObjectChoice || !ObjectChoice->TryGetStringField(TEXT("id"), ChoiceId) || !ObjectChoice->TryGetStringField(TEXT("text"), ChoiceText))
                { Error = TEXT("invalid choice fields"); return false; }
                FDialogueChoice Choice; Choice.Id = FName(*ChoiceId); Choice.Text = FText::FromString(ChoiceText);
                ObjectChoice->TryGetStringField(TEXT("next"), Next);
                ObjectChoice->TryGetStringField(TEXT("event"), Event);
                ObjectChoice->TryGetBoolField(TEXT("end"), Choice.bEndConversation);
                Choice.NextNode = FName(*Next); Choice.EventTag = FName(*Event);
                Node.Choices.Add(Choice);
            }
            Data->Nodes.Add(Node);
        }
        if (Catalog.Contains(Data->NpcId)) { Error = TEXT("duplicate NPC id"); return false; }
        if (!Data->Validate(Error)) return false;
        Catalog.Add(Data->NpcId, Data);
    }
    if (Catalog.IsEmpty()) { Error = TEXT("empty dialogue catalog"); return false; }
    Out = MoveTemp(Catalog);
    return true;
}
