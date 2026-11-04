#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "TaskValidatorSubsystem.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, TaskValidator)

// Console commands mirror the admin keys, handy for testing without a headset or keyboard focus.
namespace
{
	UTaskValidatorSubsystem* SubsystemFor(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UTaskValidatorSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs SessionCmd(
		TEXT("Validator.Session"),
		TEXT("Start a new session: Validator.Session P01"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World))
			{
				S->StartSession(Args.Num() > 0 ? Args[0] : FString());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs NextCmd(
		TEXT("Validator.Next"),
		TEXT("Start the next task"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World)) { S->StartNextTask(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs HelpCmd(
		TEXT("Validator.Intervention"),
		TEXT("Log that the participant asked for help on the running task"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World)) { S->LogIntervention(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs AbandonCmd(
		TEXT("Validator.Abandon"),
		TEXT("Participant stopped the running task (recorded as not completed)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World)) { S->AbandonActiveTask(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs CompleteCmd(
		TEXT("Validator.ForceComplete"),
		TEXT("Debug only: mark the running task completed"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World)) { S->CompleteActiveTask(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs InteractCmd(
		TEXT("Validator.FakeInteraction"),
		TEXT("Debug only: count one interaction on the running task"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTaskValidatorSubsystem* S = SubsystemFor(World)) { S->NotifyInteraction(nullptr); }
		}));
}
