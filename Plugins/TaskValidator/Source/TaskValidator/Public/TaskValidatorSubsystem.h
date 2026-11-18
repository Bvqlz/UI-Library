#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TaskValidatorTypes.h"
#include "TaskValidatorSubsystem.generated.h"

class AValidatorTask;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnValidatorTaskStarted, AValidatorTask*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnValidatorTaskEnded, const FValidatorTaskRecord&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnValidatorInteractionCounted, AActor* /*InteractedActor*/, FName /*Method*/);

/**
 * Owns the study session: which participant, which task is running, its timer and counters,
 * and the CSV file. Lives on the GameInstance so a session survives level changes.
 *
 * The validator knows nothing about any interaction system. Interaction code (a bridge, a grab,
 * a select, a poke) only needs one call, from C++ or Blueprint:
 *     UTaskValidatorSubsystem::ReportInteraction(this, SelectedActor, TEXT("Ray"));
 */
UCLASS()
class TASKVALIDATOR_API UTaskValidatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTaskValidatorSubsystem* Get(const UObject* WorldContext);

	/**
	 * Call whenever the participant grabs or selects an object. Counts as one attempt on the running task.
	 * Safe to call when no task is running (it is ignored).
	 * @param InteractedActor The actor the participant interacted with. May be null (the attempt still counts).
	 * @param Method          How they did it, e.g. Poke, Ray, Grab. Recorded in the CSV InteractionMethods column. Optional.
	 */
	UFUNCTION(BlueprintCallable, Category = "Task Validator", meta = (WorldContext = "WorldContext", AdvancedDisplay = "Method"))
	static void ReportInteraction(const UObject* WorldContext, AActor* InteractedActor, FName Method = NAME_None);

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
	/** Counts one attempt on the running task. Prefer the static ReportInteraction. See ReportInteraction for the parameters. */
	void NotifyInteraction(AActor* InteractedActor, FName Method = NAME_None);

	AValidatorTask* GetActiveTask() const { return ActiveTask.Get(); }
	double GetActiveTaskElapsed() const;
	const FValidatorTaskRecord& GetActiveRecord() const { return ActiveRecord; }

	FOnValidatorTaskStarted OnTaskStarted;
	FOnValidatorTaskEnded OnTaskEnded;
	/** Fires after an interaction was counted as an attempt (not for ignored or duplicate reports). */
	FOnValidatorInteractionCounted OnInteractionCounted;

private:
	/** True if this actor was already counted this frame, so one action seen by two sources counts once. */
	bool IsDuplicateReport(const AActor* InteractedActor);
	void FinishActiveTask(EValidatorTaskResult Result);
	void WriteRow(const FValidatorTaskRecord& Record);

	TArray<TWeakObjectPtr<AValidatorTask>> RegisteredTasks;
	TSet<TWeakObjectPtr<AValidatorTask>> FinishedTasks;
	TWeakObjectPtr<AValidatorTask> ActiveTask;
	FValidatorTaskRecord ActiveRecord;
	/** FPlatformTime::Seconds() at task start. Wall clock, so pauses and time dilation don't skew results. */
	double ActiveStartSeconds = 0.0;

	/** Actors counted in LastReportFrame. Null (an attempt with no actor) is never treated as a duplicate. */
	TArray<TWeakObjectPtr<const AActor>> ActorsReportedThisFrame;
	uint64 LastReportFrame = MAX_uint64;

	bool bSessionActive = false;
	FString ParticipantId;
	FString LogFilePath;
};
