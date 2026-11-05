#include "ValidatorTask.h"

#include "TaskValidatorSubsystem.h"

AValidatorTask::AValidatorTask()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AValidatorTask::BeginPlay()
{
	Super::BeginPlay();
	if (UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this))
	{
		Subsystem->RegisterTask(this);
	}
}

void AValidatorTask::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this))
	{
		Subsystem->UnregisterTask(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AValidatorTask::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTaskActive)
	{
		TickActiveTask(DeltaSeconds);
	}
}

void AValidatorTask::MarkConditionMet()
{
	if (!bTaskActive)
	{
		return;
	}
	if (UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this))
	{
		Subsystem->CompleteActiveTask();
	}
}

void AValidatorTask::BeginTask()
{
	bTaskActive = true;
	SetActorTickEnabled(true);
	OnTaskBegin();
}

void AValidatorTask::EndTask(EValidatorTaskResult Result, FValidatorTaskRecord& InOutRecord)
{
	bTaskActive = false;
	SetActorTickEnabled(false);
	OnTaskEnd(Result, InOutRecord);
}
