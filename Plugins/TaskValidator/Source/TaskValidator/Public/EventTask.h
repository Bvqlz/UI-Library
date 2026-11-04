#pragma once

#include "CoreMinimal.h"
#include "ValidatorTask.h"
#include "EventTask.generated.h"

/**
 * Task finished by an event instead of a spatial check, the "boolean" template:
 * - Game code calls MarkConditionMet() (button pressed, menu option chosen, ...), or
 * - the participant interacts with one of CompleteOnInteractionWith (selection tasks:
 *   "select the windmill"). Interactions with other actors still count as attempts.
 */
UCLASS()
class TASKVALIDATOR_API AEventTask : public AValidatorTask
{
	GENERATED_BODY()

public:
	AEventTask();

	UPROPERTY(EditInstanceOnly, Category = "Validator|Event")
	TArray<TObjectPtr<AActor>> CompleteOnInteractionWith;

protected:
	virtual void OnInteraction(AActor* InteractedActor) override;
};
