#include "DialogueComponent.h"
#include "DialogueManagerComponent.h"
bool UDialogueComponent::Acquire(UDialogueManagerComponent* Manager)
{
    if (!Manager || (ConversationOwner.IsValid() && ConversationOwner != Manager)) return false;
    ConversationOwner = Manager;
    return true;
}
void UDialogueComponent::Release(UDialogueManagerComponent* Manager)
{
    if (ConversationOwner == Manager) ConversationOwner.Reset();
}
void UDialogueComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    const TWeakObjectPtr<UDialogueManagerComponent> Manager = ConversationOwner;
    ConversationOwner.Reset();
    if (Manager.IsValid()) Manager->EndConversation(true);
    Super::EndPlay(Reason);
}
