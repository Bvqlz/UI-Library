/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ValidatorBridgeSubsystem.h"
#include "ButtonPanel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interactable.h"
#include "ProductHistorySubsystem.h"
#include "ProductPlacer.h"
#include "TaskValidatorSubsystem.h"

bool UValidatorBridgeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only worlds that actually run: game and PIE, not editor preview or asset worlds
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UValidatorBridgeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		Bind(*It);
	}
	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UValidatorBridgeSubsystem::HandleActorSpawned));

	if (UProductHistorySubsystem* History = InWorld.GetSubsystem<UProductHistorySubsystem>())
	{
		History->OnEditApplied.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleProductEdit);
	}
}

void UValidatorBridgeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

void UValidatorBridgeSubsystem::HandleActorSpawned(AActor* Actor)
{
	Bind(Actor);
}

void UValidatorBridgeSubsystem::Bind(AActor* Actor)
{
	// AddUnique, so binding an actor twice (it existed at start and was also seen spawning) is harmless
	if (AInteractable* Interactable = Cast<AInteractable>(Actor))
	{
		Interactable->OnInteractableStateChanged.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleInteractableState);
	}
	else if (AWristToolkit* Toolkit = Cast<AWristToolkit>(Actor))
	{
		Toolkit->OnItemActivated.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleToolkitItem);
	}
	else if (AButtonPanel* Panel = Cast<AButtonPanel>(Actor))
	{
		Panel->OnButtonPressed.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleButtonPanel);
	}
	else if (AProductPlacer* Placer = Cast<AProductPlacer>(Actor))
	{
		Placer->OnSelectionChanged.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleProductSelected);
	}
}

AActor* UValidatorBridgeSubsystem::OwnerOf(AActor* Actor)
{
	// The sample spawns its interactables at runtime and attaches them to a placed actor: the windmill and
	// the crossing each spawn a hidden logic actor, and the controller box spawns its eight buttons. A task
	// can only name actors that exist in the level, so report the placed owner, not the spawned helper
	while (Actor && Actor->GetAttachParentActor())
	{
		Actor = Actor->GetAttachParentActor();
	}
	return Actor;
}

void UValidatorBridgeSubsystem::Report(AActor* Actor) const
{
	UTaskValidatorSubsystem::ReportInteraction(this, Actor);
}

void UValidatorBridgeSubsystem::HandleInteractableState(const FInteractableStateArgs& StateArgs)
{
	// Entering the action state is the "press": hovering or touching alone is not an attempt
	if (StateArgs.NewInteractableState == EInteractableState::ActionState &&
		StateArgs.OldInteractableState != EInteractableState::ActionState)
	{
		Report(OwnerOf(StateArgs.Interactable));
	}
}

void UValidatorBridgeSubsystem::HandleToolkitItem(EWristToolkitItem /*Item*/)
{
	// The toolkit does not pass itself, so find it again; there is one per level
	AActor* Toolkit = LastToolkitOwner.Get();
	if (!Toolkit)
	{
		const TActorIterator<AWristToolkit> It(GetWorld());
		Toolkit = It ? *It : nullptr;
		LastToolkitOwner = Toolkit;
	}
	Report(Toolkit);
}

void UValidatorBridgeSubsystem::HandleButtonPanel(int32 /*ButtonIndex*/, const FText& /*Label*/)
{
	const TActorIterator<AButtonPanel> It(GetWorld());
	Report(It ? *It : nullptr);
}

void UValidatorBridgeSubsystem::HandleProductSelected(AFurnitureProduct* Product)
{
	// Clearing the selection is not an attempt
	if (Product)
	{
		Report(Product);
	}
}

void UValidatorBridgeSubsystem::HandleProductEdit(const FProductEdit& Edit, bool bWasUndo)
{
	if (bWasUndo)
	{
		return;
	}

	// Pass the product itself when it still exists, so a task can tell which one the participant touched.
	// A removed product is gone, so the attempt is counted with no actor.
	AFurnitureProduct* Product = nullptr;
	if (const UProductHistorySubsystem* History = GetWorld()->GetSubsystem<UProductHistorySubsystem>())
	{
		for (AFurnitureProduct* Candidate : History->GetProducts())
		{
			if (Candidate->ProductGuid == Edit.ProductGuid)
			{
				Product = Candidate;
				break;
			}
		}
	}
	Report(Product);
}
