/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "SelectRelayComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

USelectRelayComponent::USelectRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USelectRelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint and Interaction SDK actors may create their interactables in their own BeginPlay, which can run
	// after this one. Bind now and again next tick; binding is AddUnique, so twice is harmless.
	Rebind();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &USelectRelayComponent::Rebind));
	}
}

void USelectRelayComponent::Rebind()
{
	for (UActorComponent* Component : IsdkPointables::Find(GetOwner()))
	{
		if (IIsdkIPointable* Pointable = IsdkPointables::AsPointable(Component))
		{
			Pointable->GetInteractionPointerEventDelegate().AddUniqueDynamic(this, &USelectRelayComponent::HandlePointerEvent);
		}
	}
}

void USelectRelayComponent::HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
{
	// Pointer lifecycle: Hover -> Select -> (Unselect | Cancel) -> Unhover, per pointer. Several pointers (two hands,
	// hand and controller) can be active at once, so count them and only fire on the first in and the last out.
	const FName Method = IsdkPointables::MethodOf(IsdkPointables::SourceOf(PointerEvent));
	switch (PointerEvent.Type)
	{
	case EIsdkPointerEventType::Hover:
		if (HoverCount++ == 0 && SelectCount == 0)
		{
			OnHoverStarted.Broadcast();
		}
		break;

	case EIsdkPointerEventType::Unhover:
		HoverCount = FMath::Max(0, HoverCount - 1);
		if (HoverCount == 0 && SelectCount == 0)
		{
			OnHoverEnded.Broadcast();
		}
		break;

	case EIsdkPointerEventType::Select:
		if (SelectCount++ == 0)
		{
			OnSelected.Broadcast(Method);
		}
		break;

	case EIsdkPointerEventType::Unselect:
	case EIsdkPointerEventType::Cancel:
		if (SelectCount > 0 && --SelectCount == 0)
		{
			OnUnselected.Broadcast(Method);
		}
		break;

	default:
		// Move: the pointer moved while hovering or selecting; nothing to do
		break;
	}
}
