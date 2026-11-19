#include "EventTask.h"

#include "Components/BillboardComponent.h"

AEventTask::AEventTask()
{
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
}

void AEventTask::OnInteraction(AActor* InteractedActor)
{
	if (!InteractedActor)
	{
		return;
	}
	if (CompleteOnInteractionWith.Contains(InteractedActor) ||
		(!CompleteOnInteractionWithTag.IsNone() && InteractedActor->ActorHasTag(CompleteOnInteractionWithTag)))
	{
		MarkConditionMet();
	}
}
