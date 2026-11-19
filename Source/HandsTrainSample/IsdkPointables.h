/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"

// Meta XR Interaction SDK (OculusInteraction plugin): the pointer event payload (FIsdkInteractionPointerEvent,
// EIsdkPointerEventType) and the pointable interface (UIsdkIPointable / IIsdkIPointable).
// This is the only file in the sample that names these two headers. If the first build can't find one of them,
// search the installed plugin (Plugins/MetaXRInteraction) for the type and fix the path here.
#include "StructTypes.h"
#include "Interaction/Pointable/IsdkIPointable.h"

class AActor;
class UActorComponent;

/**
 * Helpers for working with any Interaction SDK interactable (poke, ray, grab, widget) through the one contract
 * they share: IIsdkIPointable and its pointer events. Used by the validator bridge and by USelectRelayComponent.
 */
namespace IsdkPointables
{
	/** Every component on Actor that implements IIsdkIPointable. */
	HANDSTRAINSAMPLE_API TArray<UActorComponent*> Find(const AActor* Actor);

	/** The pointable interface of a component returned by Find (bind GetInteractionPointerEventDelegate on it). */
	HANDSTRAINSAMPLE_API IIsdkIPointable* AsPointable(UActorComponent* Component);

	/** The component that raised a pointer event, or null. */
	HANDSTRAINSAMPLE_API UActorComponent* SourceOf(const FIsdkInteractionPointerEvent& PointerEvent);

	/** The interaction method for an interactable, from its native class name: Poke, Ray, Grab, ... */
	HANDSTRAINSAMPLE_API FName MethodOf(const UObject* Interactable);
}
