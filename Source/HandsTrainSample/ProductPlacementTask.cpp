/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ProductPlacementTask.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "ProductHistorySubsystem.h"

AProductPlacementTask::AProductPlacementTask()
{
	TaskGroup = TEXT("Manipulation");
	TaskName = TEXT("PlaceProduct");
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
}

void AProductPlacementTask::OnTaskBegin()
{
	ExistingAtStart.Reset();
	if (const UProductHistorySubsystem* History = GetWorld()->GetSubsystem<UProductHistorySubsystem>())
	{
		for (const AFurnitureProduct* Product : History->GetProducts())
		{
			ExistingAtStart.Add(Product->ProductGuid);
		}
	}
}

void AProductPlacementTask::OnInteraction(AActor* /*InteractedActor*/)
{
	const UProductHistorySubsystem* History = GetWorld()->GetSubsystem<UProductHistorySubsystem>();
	if (!History)
	{
		return;
	}

	int32 NewProducts = 0;
	for (const AFurnitureProduct* Product : History->GetProducts())
	{
		if (Product->Definition.Type == RequiredType && !ExistingAtStart.Contains(Product->ProductGuid))
		{
			++NewProducts;
		}
	}

	if (NewProducts >= RequiredCount)
	{
		MarkConditionMet();
	}
}
