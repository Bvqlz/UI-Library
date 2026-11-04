#pragma once

#include "CoreMinimal.h"
#include "ValidatorTask.h"
#include "TargetZoneTask.generated.h"

class UBoxComponent;

UENUM()
enum class EZoneRotationCheck : uint8
{
	/** Position only. */
	Ignore,
	/** Target's up axis must match the zone's up axis (spin around up is free). Good for "set it down upright". */
	UprightOnly,
	/** Full orientation must match the zone's. */
	Full,
};

/**
 * "Place the object here" task. Completes when TargetActor's origin is inside the box,
 * within the rotation tolerance, and has stayed there, nearly still, for DwellSeconds.
 * The box has no collision and no physics: it's a pure geometric check each tick.
 */
UCLASS()
class TASKVALIDATOR_API ATargetZoneTask : public AValidatorTask
{
	GENERATED_BODY()

public:
	ATargetZoneTask();

	/** Size and place this to set the position tolerance. */
	UPROPERTY(VisibleAnywhere, Category = "Validator|Zone")
	TObjectPtr<UBoxComponent> Zone;

	/** The object the participant has to move. */
	UPROPERTY(EditInstanceOnly, Category = "Validator|Zone")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, Category = "Validator|Zone")
	EZoneRotationCheck RotationCheck = EZoneRotationCheck::Ignore;

	UPROPERTY(EditAnywhere, Category = "Validator|Zone", meta = (ClampMin = 0, ClampMax = 180, EditCondition = "RotationCheck != EZoneRotationCheck::Ignore"))
	float RotationToleranceDegrees = 15.f;

	/** How long the target must stay inside before the task counts as done. Stops a pass-through from counting. */
	UPROPERTY(EditAnywhere, Category = "Validator|Zone", meta = (ClampMin = 0))
	float DwellSeconds = 0.5f;

	/** Target must be moving slower than this (cm/s) to count, i.e. it has been let go and settled. */
	UPROPERTY(EditAnywhere, Category = "Validator|Zone", meta = (ClampMin = 0))
	float MaxSettledSpeed = 10.f;

	/** Put the target back where it started when the task begins. */
	UPROPERTY(EditAnywhere, Category = "Validator|Zone")
	bool bResetTargetOnStart = true;

	/** Draw the box outline during play. Turn off for real sessions if a separate visible marker is used. */
	UPROPERTY(EditAnywhere, Category = "Validator|Zone")
	bool bShowZoneInGame = true;

protected:
	virtual void BeginPlay() override;
	virtual void OnTaskBegin() override;
	virtual void OnTaskEnd(EValidatorTaskResult Result, FValidatorTaskRecord& InOutRecord) override;
	virtual void TickActiveTask(float DeltaSeconds) override;

private:
	bool IsTargetPlaced(float& OutRotationErrorDeg) const;
	float ComputeRotationError() const;
	void SetZoneColor(const FColor& Color);

	FTransform TargetStartTransform;
	FVector LastTargetLocation = FVector::ZeroVector;
	float DwellTimer = 0.f;
};
