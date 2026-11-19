/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "IsdkPointables.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

TArray<UActorComponent*> IsdkPointables::Find(const AActor* Actor)
{
	TArray<UActorComponent*> Result;
	if (!IsValid(Actor))
	{
		return Result;
	}

	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->Implements<UIsdkIPointable>())
		{
			Result.Add(Component);
		}
	}
	return Result;
}

IIsdkIPointable* IsdkPointables::AsPointable(UActorComponent* Component)
{
	return Cast<IIsdkIPointable>(Component);
}

UActorComponent* IsdkPointables::SourceOf(const FIsdkInteractionPointerEvent& PointerEvent)
{
	return Cast<UActorComponent>(PointerEvent.Interactable.GetObject());
}

FName IsdkPointables::MethodOf(const UObject* Interactable)
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
