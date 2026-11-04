#include "TaskValidatorSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ValidatorTask.h"

DEFINE_LOG_CATEGORY_STATIC(LogTaskValidator, Log, All);

namespace
{
	const TCHAR* CsvHeader = TEXT("ParticipantId,TaskGroup,Task,TaskOrder,Result,Assisted,Interventions,Attempts,")
		TEXT("TimeToFirstInteraction_s,TimeOnTask_s,StartUtc,EndUtc,PositionError_cm,RotationError_deg");

	FString CsvField(const FString& Value)
	{
		if (Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")) || Value.Contains(TEXT("\n")))
		{
			return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
		}
		return Value;
	}

	/** Blank cell for "not applicable" so spreadsheets don't average in -1s. */
	FString OptionalNumber(double Value, int32 Decimals)
	{
		return Value < 0.0 ? FString() : FString::Printf(TEXT("%.*f"), Decimals, Value);
	}
}

UTaskValidatorSubsystem* UTaskValidatorSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UTaskValidatorSubsystem>() : nullptr;
}

void UTaskValidatorSubsystem::ReportInteraction(const UObject* WorldContext, AActor* InteractedActor)
{
	if (UTaskValidatorSubsystem* Subsystem = Get(WorldContext))
	{
		Subsystem->NotifyInteraction(InteractedActor);
	}
}

void UTaskValidatorSubsystem::Deinitialize()
{
	EndSession();
	Super::Deinitialize();
}

void UTaskValidatorSubsystem::StartSession(const FString& InParticipantId)
{
	EndSession();

	ParticipantId = InParticipantId.IsEmpty() ? TEXT("P00") : InParticipantId;
	FinishedTasks.Reset();

	// Never overwrite an earlier session's data: P01_validator.csv, then P01_validator_2.csv, ...
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("ValidatorLogs");
	IFileManager::Get().MakeDirectory(*Dir, true);
	LogFilePath = Dir / FString::Printf(TEXT("%s_validator.csv"), *ParticipantId);
	for (int32 Suffix = 2; IFileManager::Get().FileExists(*LogFilePath); ++Suffix)
	{
		LogFilePath = Dir / FString::Printf(TEXT("%s_validator_%d.csv"), *ParticipantId, Suffix);
	}

	FFileHelper::SaveStringToFile(FString(CsvHeader) + LINE_TERMINATOR, *LogFilePath);
	bSessionActive = true;
	UE_LOG(LogTaskValidator, Log, TEXT("Session started for %s, logging to %s"), *ParticipantId, *FPaths::ConvertRelativePathToFull(LogFilePath));
}

void UTaskValidatorSubsystem::EndSession()
{
	if (!bSessionActive)
	{
		return;
	}
	if (ActiveTask.IsValid())
	{
		AbandonActiveTask();
	}
	bSessionActive = false;
	UE_LOG(LogTaskValidator, Log, TEXT("Session for %s ended"), *ParticipantId);
}

void UTaskValidatorSubsystem::RegisterTask(AValidatorTask* Task)
{
	RegisteredTasks.AddUnique(Task);
}

void UTaskValidatorSubsystem::UnregisterTask(AValidatorTask* Task)
{
	if (ActiveTask.Get() == Task)
	{
		AbandonActiveTask();
	}
	RegisteredTasks.Remove(Task);
}

AValidatorTask* UTaskValidatorSubsystem::FindNextTask() const
{
	AValidatorTask* Best = nullptr;
	for (const TWeakObjectPtr<AValidatorTask>& Weak : RegisteredTasks)
	{
		AValidatorTask* Task = Weak.Get();
		if (!Task || FinishedTasks.Contains(Weak))
		{
			continue;
		}
		if (!Best || Task->Order < Best->Order || (Task->Order == Best->Order && Task->TaskName < Best->TaskName))
		{
			Best = Task;
		}
	}
	return Best;
}

int32 UTaskValidatorSubsystem::GetRemainingTaskCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AValidatorTask>& Weak : RegisteredTasks)
	{
		Count += (Weak.IsValid() && !FinishedTasks.Contains(Weak)) ? 1 : 0;
	}
	return Count;
}

bool UTaskValidatorSubsystem::StartNextTask()
{
	if (ActiveTask.IsValid())
	{
		return false;
	}
	AValidatorTask* Next = FindNextTask();
	if (!Next)
	{
		return false;
	}
	StartTask(Next);
	return true;
}

void UTaskValidatorSubsystem::StartTask(AValidatorTask* Task)
{
	if (!Task || !bSessionActive)
	{
		return;
	}
	if (ActiveTask.IsValid())
	{
		AbandonActiveTask();
	}

	ActiveRecord = FValidatorTaskRecord();
	ActiveRecord.ParticipantId = ParticipantId;
	ActiveRecord.TaskGroup = Task->TaskGroup;
	ActiveRecord.TaskName = Task->TaskName;
	ActiveRecord.TaskOrder = Task->Order;
	ActiveRecord.StartUtc = FDateTime::UtcNow();
	ActiveStartSeconds = FPlatformTime::Seconds();
	ActiveTask = Task;

	Task->BeginTask();
	UE_LOG(LogTaskValidator, Log, TEXT("Started task %s / %s"), *Task->TaskGroup, *Task->TaskName);
	OnTaskStarted.Broadcast(Task);
}

void UTaskValidatorSubsystem::CompleteActiveTask()
{
	FinishActiveTask(EValidatorTaskResult::Completed);
}

void UTaskValidatorSubsystem::AbandonActiveTask()
{
	FinishActiveTask(EValidatorTaskResult::Abandoned);
}

void UTaskValidatorSubsystem::LogIntervention()
{
	if (ActiveTask.IsValid())
	{
		++ActiveRecord.Interventions;
		UE_LOG(LogTaskValidator, Log, TEXT("Intervention logged (%d)"), ActiveRecord.Interventions);
	}
}

void UTaskValidatorSubsystem::NotifyInteraction(AActor* InteractedActor)
{
	AValidatorTask* Task = ActiveTask.Get();
	if (!Task)
	{
		return;
	}

	++ActiveRecord.Attempts;
	if (ActiveRecord.TimeToFirstInteraction < 0.0)
	{
		ActiveRecord.TimeToFirstInteraction = GetActiveTaskElapsed();
	}
	Task->OnInteraction(InteractedActor);
}

double UTaskValidatorSubsystem::GetActiveTaskElapsed() const
{
	return ActiveTask.IsValid() ? FPlatformTime::Seconds() - ActiveStartSeconds : 0.0;
}

void UTaskValidatorSubsystem::FinishActiveTask(EValidatorTaskResult Result)
{
	AValidatorTask* Task = ActiveTask.Get();
	if (!Task)
	{
		return;
	}

	ActiveRecord.Result = Result;
	ActiveRecord.TimeOnTask = GetActiveTaskElapsed();
	ActiveRecord.EndUtc = FDateTime::UtcNow();

	// Clear first so anything the task does while ending can't re-enter this function.
	ActiveTask.Reset();
	FinishedTasks.Add(Task);
	Task->EndTask(Result, ActiveRecord);

	WriteRow(ActiveRecord);
	UE_LOG(LogTaskValidator, Log, TEXT("Task %s %s in %.2fs, %d attempts, %d interventions"),
		*ActiveRecord.TaskName, LexToString(Result), ActiveRecord.TimeOnTask, ActiveRecord.Attempts, ActiveRecord.Interventions);
	OnTaskEnded.Broadcast(ActiveRecord);
}

void UTaskValidatorSubsystem::WriteRow(const FValidatorTaskRecord& R)
{
	if (LogFilePath.IsEmpty())
	{
		return;
	}

	const FString Row = FString::Join(TArray<FString>{
		CsvField(R.ParticipantId),
		CsvField(R.TaskGroup),
		CsvField(R.TaskName),
		FString::FromInt(R.TaskOrder),
		LexToString(R.Result),
		R.WasAssisted() ? TEXT("1") : TEXT("0"),
		FString::FromInt(R.Interventions),
		FString::FromInt(R.Attempts),
		OptionalNumber(R.TimeToFirstInteraction, 3),
		FString::Printf(TEXT("%.3f"), R.TimeOnTask),
		R.StartUtc.ToIso8601(),
		R.EndUtc.ToIso8601(),
		OptionalNumber(R.PositionErrorCm, 2),
		OptionalNumber(R.RotationErrorDeg, 2),
	}, TEXT(","));

	// Append one row per task so a crash mid-session loses at most the running task.
	FFileHelper::SaveStringToFile(Row + LINE_TERMINATOR, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), FILEWRITE_Append);
}
