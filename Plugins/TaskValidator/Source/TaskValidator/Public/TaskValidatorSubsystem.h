#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TaskValidatorTypes.h"
#include "TaskValidatorSubsystem.generated.h"

class AValidatorTask;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnValidatorTaskStarted, AValidatorTask*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnValidatorTaskEnded, const FValidatorTaskRecord&);

/**
 * Owns the study session: which participant, which task is running, its timer and counters,
 * and the CSV file. Lives on the GameInstance so a session survives level changes.
 *
 * Interaction code (grab, select, poke) only needs one call:
 *     UTaskValidatorSubsystem::ReportInteraction(this, GrabbedActor);
 */
UCLASS()
class TASKVALIDATOR_API UTaskValidatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTaskValidatorSubsystem* Get(const UObject* WorldContext);

	/** Call whenever the participant grabs or selects an object. Counts as an attempt on the running task. Safe to call when no task is running. */
	static void ReportInteraction(const UObject* WorldContext, AActor* InteractedActor);

	virtual void Deinitialize() override;

	/** Starts a new session and CSV file. Ends the current session first if one is running. */
	void StartSession(const FString& InParticipantId);
	/** Abandons the running task (if any) and closes the session. */
	void EndSession();
	bool IsSessionActive() const { return bSessionActive; }
	const FString& GetParticipantId() const { return ParticipantId; }
	const FString& GetLogFilePath() const { return LogFilePath; }

	void RegisterTask(AValidatorTask* Task);
	void UnregisterTask(AValidatorTask* Task);

	/** Lowest Order task in the current world that hasn't run this session. */
	AValidatorTask* FindNextTask() const;
	int32 GetRemainingTaskCount() const;
	/** Starts FindNextTask(). Returns false if a task is already running or none are left. */
	bool StartNextTask();
	void StartTask(AValidatorTask* Task);

	/** Called by a task when its success condition is met. */
	void CompleteActiveTask();
	/** The participant stopped the task. */
	void AbandonActiveTask();
	/** The administrator helped the participant. Marks the running task as assisted. */
	void LogIntervention();
	void NotifyInteraction(AActor* InteractedActor);

	AValidatorTask* GetActiveTask() const { return ActiveTask.Get(); }
	double GetActiveTaskElapsed() const;
	const FValidatorTaskRecord& GetActiveRecord() const { return ActiveRecord; }

	FOnValidatorTaskStarted OnTaskStarted;
	FOnValidatorTaskEnded OnTaskEnded;

private:
	void FinishActiveTask(EValidatorTaskResult Result);
	void WriteRow(const FValidatorTaskRecord& Record);

	TArray<TWeakObjectPtr<AValidatorTask>> RegisteredTasks;
	TSet<TWeakObjectPtr<AValidatorTask>> FinishedTasks;
	TWeakObjectPtr<AValidatorTask> ActiveTask;
	FValidatorTaskRecord ActiveRecord;
	/** FPlatformTime::Seconds() at task start. Wall clock, so pauses and time dilation don't skew results. */
	double ActiveStartSeconds = 0.0;

	bool bSessionActive = false;
	FString ParticipantId;
	FString LogFilePath;
};
