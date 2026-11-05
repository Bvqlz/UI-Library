/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "FurnitureProduct.h"
#include "Interactable.h"
#include "ProductHistorySubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "WristToolkit.h"
#include "ValidatorBridgeSubsystem.generated.h"

/**
 * The only place where the HandTrackingTools plugin and the TaskValidator plugin meet.
 *
 * Neither plugin knows about the other. HandTrackingTools raises events when a participant touches
 * something (a poke button fires, a ray selects, a wrist item is poked, a product is selected or
 * edited). This subsystem listens to those events and turns each one into the validator's single
 * input call:
 *
 *     UTaskValidatorSubsystem::ReportInteraction(this, TheActorTouched);
 *
 * That call counts an attempt on whatever task is running and sets the time to first interaction.
 * It does nothing when no task is running, so the bridge is harmless outside a study.
 *
 * It needs no setup. It is created with every game world, binds to what is already in the level, and
 * binds to actors spawned later. To validate a different interaction in another project, write a
 * bridge like this one; the plugins do not change.
 */
UCLASS()
class HANDSTRAINSAMPLE_API UValidatorBridgeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void Bind(AActor* Actor);
	void HandleActorSpawned(AActor* Actor);

	/** Poke buttons, ray targets and the like: the participant "pressed" it. */
	UFUNCTION()
	void HandleInteractableState(const FInteractableStateArgs& StateArgs);

	UFUNCTION()
	void HandleToolkitItem(EWristToolkitItem Item);

	UFUNCTION()
	void HandleButtonPanel(int32 ButtonIndex, const FText& Label);

	UFUNCTION()
	void HandleProductSelected(AFurnitureProduct* Product);

	/** Place, remove, move, rotate and resize all count. Undo does not. */
	UFUNCTION()
	void HandleProductEdit(const FProductEdit& Edit, bool bWasUndo);

	/** The placed actor an interactable belongs to: the root of its attachment chain, or itself. */
	static AActor* OwnerOf(AActor* Actor);

	void Report(AActor* Actor) const;

	FDelegateHandle ActorSpawnedHandle;
	TWeakObjectPtr<AActor> LastToolkitOwner;
};
