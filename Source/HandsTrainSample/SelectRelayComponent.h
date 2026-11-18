/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsdkPointables.h"
#include "SelectRelayComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSelectRelayEvent, FName, Method);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSelectRelayHoverEvent);

/**
 * Turns the Interaction SDK interactables on its actor into simple gameplay events.
 *
 * Add it to an actor next to one or more Interaction SDK interactables (IsdkRayInteractable, IsdkPokeInteractable,
 * a grabbable, ...). It listens to all of them through their shared pointer events and raises:
 *   OnSelected / OnUnselected  the first pointer selects / the last pointer lets go (pinch, poke press, grab)
 *   OnHoverStarted / OnHoverEnded  the first pointer hovers / the last pointer leaves
 *
 * Gameplay code reacts to these and never needs to know which interaction was used. This replaces the custom
 * AInteractable / AInteractableTool state machine for scene objects.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class HANDSTRAINSAMPLE_API USelectRelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USelectRelayComponent();

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSelectRelayEvent OnSelected;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSelectRelayEvent OnUnselected;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSelectRelayHoverEvent OnHoverStarted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSelectRelayHoverEvent OnHoverEnded;

	/** At least one pointer is hovering or selecting. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHovered() const { return HoverCount > 0 || SelectCount > 0; }

	/** At least one pointer is selecting (pinching, pressing, grabbing). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsSelected() const { return SelectCount > 0; }

	/** Bind to the owner's interactables again, for interactables added after BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Rebind();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);

	int32 HoverCount = 0;
	int32 SelectCount = 0;
};
