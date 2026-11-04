#include "TaskValidatorDirector.h"

#include "Components/BillboardComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "TaskValidatorSubsystem.h"
#include "ValidatorTask.h"

ATaskValidatorDirector::ATaskValidatorDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
}

void ATaskValidatorDirector::BeginPlay()
{
	Super::BeginPlay();

	FString Id = ParticipantId;
	FParse::Value(FCommandLine::Get(), TEXT("Participant="), Id);
	if (UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this))
	{
		Subsystem->StartSession(Id);
	}
}

void ATaskValidatorDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this))
	{
		Subsystem->EndSession();
	}
	Super::EndPlay(EndPlayReason);
}

void ATaskValidatorDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HandleKeys();
	if (bShowAdminOverlay)
	{
		DrawOverlay();
	}
}

void ATaskValidatorDirector::HandleKeys()
{
	// Polling raw key state keeps this independent of the project's input setup (Enhanced Input or not).
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this);
	if (!PC || !Subsystem)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(NextTaskKey))
	{
		Subsystem->StartNextTask();
	}
	if (PC->WasInputKeyJustPressed(InterventionKey))
	{
		Subsystem->LogIntervention();
	}
	if (PC->WasInputKeyJustPressed(AbandonKey))
	{
		Subsystem->AbandonActiveTask();
	}
}

void ATaskValidatorDirector::DrawOverlay() const
{
	const UTaskValidatorSubsystem* Subsystem = UTaskValidatorSubsystem::Get(this);
	if (!GEngine || !Subsystem)
	{
		return;
	}

	// Fixed keys so each line is replaced every frame instead of stacking.
	const uint64 BaseKey = 0x7A5C0000ull;
	const FColor Header(120, 200, 255);
	GEngine->AddOnScreenDebugMessage(BaseKey + 0, 0.f, Header,
		FString::Printf(TEXT("[Validator] %s   log: %s"), *Subsystem->GetParticipantId(), *FPaths::GetCleanFilename(Subsystem->GetLogFilePath())));

	if (const AValidatorTask* Active = Subsystem->GetActiveTask())
	{
		const FValidatorTaskRecord& R = Subsystem->GetActiveRecord();
		GEngine->AddOnScreenDebugMessage(BaseKey + 1, 0.f, FColor::Yellow,
			FString::Printf(TEXT("RUNNING  %s / %s   %.1fs   attempts %d   help %d"),
				*Active->TaskGroup, *Active->TaskName, Subsystem->GetActiveTaskElapsed(), R.Attempts, R.Interventions));
		GEngine->AddOnScreenDebugMessage(BaseKey + 2, 0.f, FColor::White,
			FString::Printf(TEXT("%s = help    %s = participant stopped"), *InterventionKey.GetDisplayName().ToString(), *AbandonKey.GetDisplayName().ToString()));
	}
	else if (const AValidatorTask* Next = Subsystem->FindNextTask())
	{
		GEngine->AddOnScreenDebugMessage(BaseKey + 1, 0.f, FColor::Green,
			FString::Printf(TEXT("NEXT  %s / %s   (%d left)   read instruction, then press %s"),
				*Next->TaskGroup, *Next->TaskName, Subsystem->GetRemainingTaskCount(), *NextTaskKey.GetDisplayName().ToString()));
		GEngine->AddOnScreenDebugMessage(BaseKey + 2, 0.f, FColor::White,
			FString::Printf(TEXT("\"%s\""), *Next->Instruction.ToString()));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(BaseKey + 1, 0.f, FColor::Green, TEXT("All tasks done. CSV saved."));
		GEngine->AddOnScreenDebugMessage(BaseKey + 2, 0.f, FColor::White, TEXT(""));
	}
}
