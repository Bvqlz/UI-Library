#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "TaskValidatorDirector.generated.h"

/**
 * Drop one in a study level. Starts the session on BeginPlay and gives the administrator
 * keyboard controls plus a status overlay on the PC mirror window.
 * Participant ID comes from the -Participant=P01 command line argument if given, else ParticipantId.
 */
UCLASS()
class TASKVALIDATOR_API ATaskValidatorDirector : public AActor
{
	GENERATED_BODY()

public:
	ATaskValidatorDirector();

	UPROPERTY(EditAnywhere, Category = "Validator")
	FString ParticipantId = TEXT("P00");

	/** Start the next task (press after reading its instruction). */
	UPROPERTY(EditAnywhere, Category = "Validator|Keys")
	FKey NextTaskKey = EKeys::N;

	/** Participant asked for help. Marks the running task as assisted. */
	UPROPERTY(EditAnywhere, Category = "Validator|Keys")
	FKey InterventionKey = EKeys::H;

	/** Participant chose to stop the task. Recorded as not completed. */
	UPROPERTY(EditAnywhere, Category = "Validator|Keys")
	FKey AbandonKey = EKeys::X;

	UPROPERTY(EditAnywhere, Category = "Validator")
	bool bShowAdminOverlay = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void HandleKeys();
	void DrawOverlay() const;
};
