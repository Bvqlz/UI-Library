/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "FurnitureProduct.h"
#include "ValidatorTask.h"
#include "ProductPlacementTask.generated.h"

class UProductHistorySubsystem;

/**
 * The sample's initial test task, written as a task of the sample: "place a <product> in the marked area".
 *
 * It is a normal TaskValidator task (a subclass of AValidatorTask), placed in the level and configured in
 * the Details panel. It lives in the sample, not in either plugin, because "what counts as done" depends on
 * the sample's own content (the furniture products and the placement area). That is the pattern for every
 * test task: the plugins supply the machinery, the sample supplies the task.
 *
 * Done when the participant places a product of RequiredType that did not exist when the task started. Products
 * already in the level, and products that were selected but not placed, never complete it.
 *
 * The validator calls OnInteraction (after counting the attempt) each time the bridge reports one, so the
 * attempt that places the product is always counted before the task completes.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AProductPlacementTask : public AValidatorTask
{
	GENERATED_BODY()

public:
	AProductPlacementTask();

	/** The kind of product the participant must place. */
	UPROPERTY(EditAnywhere, Category = "Validator|Placement")
	EFurnitureType RequiredType = EFurnitureType::Chair;

	/** How many new products of that kind must exist. */
	UPROPERTY(EditAnywhere, Category = "Validator|Placement", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

protected:
	virtual void OnTaskBegin() override;
	virtual void OnInteraction(AActor* InteractedActor) override;

private:
	/** Products that existed when the task began. They cannot complete it. */
	TSet<FGuid> ExistingAtStart;
};
