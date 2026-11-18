/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainCrossing.generated.h"

class USelectRelayComponent;

/**
 * Far-field prop: select it (ray + pinch, or any Interaction SDK interaction) to blink the lights and ring.
 * The interaction comes from the Interaction SDK interactable(s) added to the Blueprint, typically an
 * IsdkRayInteractable placed at CollidableInteractAnchor; SelectRelay turns their pointer events into OnSelected.
 */
UCLASS()
class HANDSTRAINSAMPLE_API ATrainCrossing : public AActor
{
	GENERATED_BODY()

public:
	ATrainCrossing();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ActivateTrainCrossing();

	UFUNCTION()
	void HandleSelected(FName Method);

protected:
	// This is a blueprint event because it's easy to do delays in blueprints.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void BlinkLights(float TimeLeftUntilNextBlink, float LightBlinkDuration, float TimeLeftForAnimation);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ToggleLightObjects(bool enableState);

	// since this is creating from an fbx scene object, find
	// components dynamically during initialization
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
	class UStaticMeshComponent* RRXingMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
	class UStaticMeshComponent* SelectionMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
	class UStaticMeshComponent* Light1Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
	class UStaticMeshComponent* Light2Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USelectionCylinderHelper* SelectionCylinderHelperComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* AudioComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* CollidableAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* CrossingSound;

	UPROPERTY(BlueprintReadWrite)
	bool BlinkFirstLight;

	/** Listens to the Interaction SDK interactables on this actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USelectRelayComponent* SelectRelay;

	virtual void BeginPlay() override;
};
