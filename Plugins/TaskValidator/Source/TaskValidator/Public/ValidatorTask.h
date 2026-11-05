#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TaskValidatorTypes.h"
#include "ValidatorTask.generated.h"

/**
 * Base class for one study task placed in a level. Subclasses decide what "done" means
 * and call MarkConditionMet(). The subsystem owns timing and counting.
 * Only ticks while the task is running.
 */
UCLASS(Abstract)
class TASKVALIDATOR_API AValidatorTask : public AActor
{
	GENERATED_BODY()

public:
	AValidatorTask();

	/** Written to the CSV TaskGroup column, e.g. "Manipulation". */
	UPROPERTY(EditAnywhere, Category = "Validator")
	FString TaskGroup = TEXT("Default");

	/** Written to the CSV Task column, e.g. "CubeToMarker". */
	UPROPERTY(EditAnywhere, Category = "Validator")
	FString TaskName = TEXT("Task");

	/** Tasks run in ascending Order (study-strategy: fixed order within a group). */
	UPROPERTY(EditAnywhere, Category = "Validator")
	int32 Order = 0;

	/** What the administrator reads aloud (Script A). Shown on the admin overlay. */
	UPROPERTY(EditAnywhere, Category = "Validator", meta = (MultiLine = true))
	FText Instruction;

	bool IsTaskActive() const { return bTaskActive; }

	/** Ends the running task as Completed. Call this from any C++ when the task's goal is reached. */
	void MarkConditionMet();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void OnTaskBegin() {}
	/** Subclasses can add metrics to the record (e.g. placement error). */
	virtual void OnTaskEnd(EValidatorTaskResult Result, FValidatorTaskRecord& InOutRecord) {}
	virtual void TickActiveTask(float DeltaSeconds) {}
	virtual void OnInteraction(AActor* InteractedActor) {}

private:
	friend class UTaskValidatorSubsystem;
	void BeginTask();
	void EndTask(EValidatorTaskResult Result, FValidatorTaskRecord& InOutRecord);

	bool bTaskActive = false;
};
