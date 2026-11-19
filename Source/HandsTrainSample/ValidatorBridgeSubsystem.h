/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "IsdkPointables.h"
#include "ProductHistorySubsystem.h"
#include "Subsystems/WorldSubsystem.h"

#include "ValidatorBridgeSubsystem.generated.h"

class AFurnitureProduct;

/**
 * The only place where interaction code and the TaskValidator plugin meet.
 *
 * The validator knows nothing about hands. This subsystem listens to interactions and turns each one into the
 * validator's single input call:
 *
 *     UTaskValidatorSubsystem::ReportInteraction(this, TheActorInteractedWith, Method);
 *
 * That call counts an attempt on whatever task is running, sets the time to first interaction and records the
 * method. It does nothing when no task is running, so the bridge is harmless outside a study.
 *
 * Sources it listens to:
 *
 * 1. Interaction SDK (automatic, general). Every component that implements IIsdkIPointable (poke, ray and grab
 *    interactables, interactable widgets) on every actor, present at start or spawned later. A Select pointer event
 *    is one attempt; Hover, Move, Unselect and Cancel are not. The method is the interactable's type
 *    (IsdkPokeInteractable -> Poke, IsdkRayInteractable -> Ray, IsdkGrabbableComponent -> Grab).
 *    No setup: add an Interaction SDK interactable to an actor and it is counted.
 *
 * 2. Sample content that is not a gesture: a furniture product selected (AProductPlacer) and a product edit
 *    applied (UProductHistorySubsystem). These keep the sample's placement task working.
 *
 * 3. Anything else: call UValidatorBridgeSubsystem::Report(this, Actor, Method) from C++ or Blueprint.
 *
 * Which actor is reported: the placed owner, i.e. the root of the attachment chain, because a task can only name
 * actors that exist in the level. Put the tag ValidatorTarget on an actor in the chain to report that actor instead.
 *
 * It needs no setup. It is created with every game world. To validate interactions in another project, copy this
 * bridge and change the sources in section 2; the plugins do not change.
 */
UCLASS()
class HANDSTRAINSAMPLE_API UValidatorBridgeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Actor tag that stops the walk up the attachment chain: the tagged actor is reported. */
	static const FName TargetTag;

	/**
	 * Report one interaction to the validator from any source.
	 * @param Actor         What the participant interacted with. May be null (the attempt still counts).
	 * @param Method        How, e.g. Poke, Ray, Grab. Optional.
	 * @param bResolveOwner Report the placed owner of Actor (see the class comment) instead of Actor itself.
	 */
	UFUNCTION(BlueprintCallable, Category = "Task Validator", meta = (WorldContext = "WorldContext", AdvancedDisplay = "bResolveOwner"))
	static void Report(const UObject* WorldContext, AActor* Actor, FName Method = NAME_None, bool bResolveOwner = true);

	/**
	 * Bind to an actor's Interaction SDK interactables. Done automatically for actors present at start and actors
	 * spawned later; call it for an actor that adds interactable components after it was spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Task Validator")
	void BindActor(AActor* Actor);

	/** The actor a report about Actor names: the first ValidatorTarget-tagged actor up the chain, else the root. */
	static AActor* ResolveReportedActor(AActor* Actor);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void BindAllActors();
	void HandleActorSpawned(AActor* Actor);

	// Source 1: Interaction SDK
	void BindIsdkPointables(AActor* Actor);

	UFUNCTION()
	void HandleIsdkPointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);

	// Source 2: sample content
	void BindSampleContent(AActor* Actor);

	UFUNCTION()
	void HandleProductSelected(AFurnitureProduct* Product);

	/** Place, remove, move, rotate and resize all count. Undo does not. */
	UFUNCTION()
	void HandleProductEdit(const FProductEdit& Edit, bool bWasUndo);

	FDelegateHandle ActorSpawnedHandle;
};
