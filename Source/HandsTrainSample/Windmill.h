/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LatentActions.h"
#include "Windmill.generated.h"

class USelectRelayComponent;

/**
 * Far-field prop: select it (ray + pinch, or any Interaction SDK interaction) to start or stop the blades.
 * The interaction itself comes from the Interaction SDK interactable(s) added to the Blueprint, typically an
 * IsdkRayInteractable placed at CollidableInteractAnchor. SelectRelay turns their pointer events into
 * OnSelected and hover, so this class doesn't depend on which interaction is used.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AWindmill : public AActor
{
	GENERATED_BODY()

public:
	AWindmill();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Movement")
	void NewMoveState(bool IsMovingNow);

	UFUNCTION()
	void HandleSelected(FName Method);

protected:
	virtual void BeginPlay() override;

	// lerp speed from start time to end time
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Movement")
	void ChangeSpeed(float SpeedChangeDuration, float TimeLeftForSpeedChange, float StartSpeed, float TargetSpeed);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	float CurrentSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
	bool IsMoving;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USelectionCylinderHelper* SelectionCylinderHelper;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* SelectionMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* BladesMesh;

	/** Listens to the Interaction SDK interactables on this actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USelectRelayComponent* SelectRelay;

	/** Where the interactable surface sits (place the IsdkRayInteractable here in the Blueprint). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* CollidableInteractAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxBladesSpeed;

private:
	float RotationAngle;
	FQuat OriginalRotation;
};
