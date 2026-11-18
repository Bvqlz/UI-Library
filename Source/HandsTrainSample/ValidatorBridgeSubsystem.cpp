/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ValidatorBridgeSubsystem.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ProductPlacer.h"
#include "TaskValidatorSubsystem.h"
#include "TimerManager.h"

const FName UValidatorBridgeSubsystem::TargetTag(TEXT("ValidatorTarget"));

bool UValidatorBridgeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only worlds that actually run: game and PIE, not editor preview or asset worlds
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UValidatorBridgeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UValidatorBridgeSubsystem::HandleActorSpawned));

	if (UProductHistorySubsystem* History = InWorld.GetSubsystem<UProductHistorySubsystem>())
	{
		History->OnEditApplied.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleProductEdit);
	}

	// Subsystems begin play before the level's actors do, and some actors (Interaction SDK widgets, Blueprints)
	// create their interactable components in BeginPlay. Bind now, and again next tick once those exist.
	// Binding is AddUnique, so binding twice is harmless.
	BindAllActors();
	InWorld.GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UValidatorBridgeSubsystem::BindAllActors));
}

void UValidatorBridgeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

void UValidatorBridgeSubsystem::BindAllActors()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		BindActor(*It);
	}
}

void UValidatorBridgeSubsystem::HandleActorSpawned(AActor* Actor)
{
	BindActor(Actor);
}

void UValidatorBridgeSubsystem::BindActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	BindIsdkPointables(Actor);
	BindSampleContent(Actor);
}

// ---------------------------------------------------------------------------------------------------------------
// Reporting

void UValidatorBridgeSubsystem::Report(const UObject* WorldContext, AActor* Actor, FName Method, bool bResolveOwner)
{
	UTaskValidatorSubsystem::ReportInteraction(WorldContext, bResolveOwner ? ResolveReportedActor(Actor) : Actor, Method);
}

AActor* UValidatorBridgeSubsystem::ResolveReportedActor(AActor* Actor)
{
	// Interactables are often on a spawned helper or a child actor attached to a placed actor (a button on a panel,
	// a logic actor on the windmill). A task can only name actors placed in the level, so walk up to the placed
	// owner. A ValidatorTarget tag on the way up stops the walk, for when the child itself is what a task names.
	for (AActor* Current = Actor; Current; Current = Current->GetAttachParentActor())
	{
		if (Current->ActorHasTag(TargetTag) || !Current->GetAttachParentActor())
		{
			return Current;
		}
	}
	return nullptr;
}

FName UValidatorBridgeSubsystem::MethodFromInteractable(const UObject* Interactable)
{
	if (!Interactable)
	{
		return NAME_None;
	}

	// Use the native class, so a Blueprint subclass of IsdkRayInteractable still reads as Ray
	const UClass* Class = Interactable->GetClass();
	while (Class && !Class->HasAnyClassFlags(CLASS_Native))
	{
		Class = Class->GetSuperClass();
	}

	// IsdkPokeInteractable -> Poke, IsdkRayInteractable -> Ray, IsdkGrabbableComponent -> Grab
	FString Name = Class ? Class->GetName() : FString();
	Name.RemoveFromStart(TEXT("Isdk"));
	Name.RemoveFromEnd(TEXT("Component"));
	Name.RemoveFromEnd(TEXT("Interactable"));
	Name.RemoveFromEnd(TEXT("bable")); // Grabbable -> Grab
	return Name.IsEmpty() ? NAME_None : FName(*Name);
}

// ---------------------------------------------------------------------------------------------------------------
// Source 1: Interaction SDK

void UValidatorBridgeSubsystem::BindIsdkPointables(AActor* Actor)
{
	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Component : Components)
	{
		if (!Component || !Component->Implements<UIsdkIPointable>())
		{
			continue;
		}
		if (IIsdkIPointable* Pointable = Cast<IIsdkIPointable>(Component))
		{
			Pointable->GetInteractionPointerEventDelegate().AddUniqueDynamic(
				this, &UValidatorBridgeSubsystem::HandleIsdkPointerEvent);
		}
	}
}

void UValidatorBridgeSubsystem::HandleIsdkPointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
{
	// Select is the press, the pinch or the grab: the study's "grabbed or selected an object".
	// Hover, Move, Unselect and Cancel are not attempts.
	if (PointerEvent.Type != EIsdkPointerEventType::Select)
	{
		return;
	}

	UObject* Interactable = PointerEvent.Interactable.GetObject();
	const UActorComponent* Component = Cast<UActorComponent>(Interactable);
	Report(this, Component ? Component->GetOwner() : nullptr, MethodFromInteractable(Interactable));
}

// ---------------------------------------------------------------------------------------------------------------
// Source 2: sample content (not gestures: these fire however the participant chose the product)

void UValidatorBridgeSubsystem::BindSampleContent(AActor* Actor)
{
	if (AProductPlacer* Placer = Cast<AProductPlacer>(Actor))
	{
		Placer->OnSelectionChanged.AddUniqueDynamic(this, &UValidatorBridgeSubsystem::HandleProductSelected);
	}
}

void UValidatorBridgeSubsystem::HandleProductSelected(AFurnitureProduct* Product)
{
	// Clearing the selection is not an attempt. The product is placed in the level, so report it as is.
	if (Product)
	{
		Report(this, Product, NAME_None, /*bResolveOwner=*/ false);
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
			if (Candidate && Candidate->ProductGuid == Edit.ProductGuid)
			{
				Product = Candidate;
				break;
			}
		}
	}
	Report(this, Product, NAME_None, /*bResolveOwner=*/ false);
}
