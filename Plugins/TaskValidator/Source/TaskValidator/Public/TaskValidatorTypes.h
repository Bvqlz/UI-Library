#pragma once

#include "CoreMinimal.h"

/** How a task ended. */
enum class EValidatorTaskResult : uint8
{
	Completed,
	/** The participant chose to stop (study-strategy section 5). Recorded as not completed. */
	Abandoned,
};

inline const TCHAR* LexToString(EValidatorTaskResult Result)
{
	return Result == EValidatorTaskResult::Completed ? TEXT("Completed") : TEXT("Abandoned");
}

/** One row of the validator CSV. Times are in seconds, distances in cm, angles in degrees. Negative means "not applicable". */
struct FValidatorTaskRecord
{
	FString ParticipantId;
	FString TaskGroup;
	FString TaskName;
	int32 TaskOrder = 0;

	EValidatorTaskResult Result = EValidatorTaskResult::Abandoned;
	int32 Interventions = 0;
	int32 Attempts = 0;
	double TimeToFirstInteraction = -1.0;
	double TimeOnTask = 0.0;

	FDateTime StartUtc;
	FDateTime EndUtc;

	float PositionErrorCm = -1.f;
	float RotationErrorDeg = -1.f;

	bool WasAssisted() const { return Interventions > 0; }
};
