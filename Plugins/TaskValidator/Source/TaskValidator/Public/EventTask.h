#pragma once

#include "CoreMinimal.h"
#include "ValidatorTask.h"
#include "EventTask.generated.h"

/**
 * Task finished by an event instead of a spatial check, the "boolean" template:
 * - Game code or Blueprint calls MarkConditionMet() (menu option chosen, ...), or
 * - the participant interacts with one of CompleteOnInteractionWith (selection tasks:
 *   "select the windmill"), or
 * - the participant interacts with an actor that has CompleteOnInteractionWithTag, which also
 *   covers actors that only exist at runtime ("press the start button").
 * Interactions with other actors still count as attempts.
 */
UCLASS()
class TASKVALIDATOR_API AEventTask : public AValidatorTask
{
	GENERATED_BODY()

public:
	AEventTask();

	/** Placed actors that complete the task when the participant interacts with them. */
	UPROPERTY(EditInstanceOnly, Category = "Validator|Event")
	TArray<TObjectPtr<AActor>> CompleteOnInteractionWith;

	/** Any actor with this tag completes the task when the participant interacts with it. None turns it off. */
	UPROPERTY(EditAnywhere, Category = "Validator|Event")
	FName CompleteOnInteractionWithTag;

protected:
	virtual void OnInteraction(AActor* InteractedActor) override;
};
