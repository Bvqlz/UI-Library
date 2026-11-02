/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "FurnitureProduct.h"
#include "GameFramework/Actor.h"
#include "OculusXRInputFunctionLibrary.h"
#include "ProductPlacer.generated.h"

class AFurnitureProduct;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlacerSelectionChanged, AFurnitureProduct*, SelectedProduct);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlacerPlacingChanged, bool, bIsPlacing);

/**
 * Pointing, selection and placement. A ray from the ray hand (or the camera's gaze when no hand is
 * tracked) hovers products; a pinch and release selects one. Choosing a product to place shows a
 * ghost on the placement area, green when valid and red with a reason when not. A fresh pinch and
 * release confirms. Selecting from the catalog never also confirms.
 *
 * Editor fallback without a headset: Space = pinch and release, Escape = cancel.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AProductPlacer : public AActor
{
	GENERATED_BODY()

public:
	AProductPlacer();

	virtual void Tick(float DeltaTime) override;

	/** The hand that points and pinches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	EOculusXRHandType RayHand = EOculusXRHandType::HandRight;

	/** Center of the marked placement area on the floor (cm). Z is the floor height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FVector PlacementAreaCenter = FVector(220.0f, 0.0f, 0.0f);

	/** Half size of the placement area in X and Y (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FVector2D PlacementAreaHalfExtent = FVector2D(80.0f, 100.0f);

	/** Positions snap to this grid (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	float GridStepCm = 5.0f;

	/** Yaw new products are placed with. 180 makes the front face a viewer standing at the origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	float PlacementYaw = 180.0f;

	/** Pinch starts when thumb and index tips are closer than this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gesture")
	float PinchStartDistance = 2.0f;

	/** Pinch ends when the tips separate beyond this (cm). Larger than the start distance to avoid flicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gesture")
	float PinchReleaseDistance = 3.5f;

	UPROPERTY(BlueprintAssignable, Category = "Placement")
	FPlacerSelectionChanged OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Placement")
	FPlacerPlacingChanged OnPlacingChanged;

	/** Show a ghost of this product and wait for a fresh pinch and release to place it. */
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void BeginPlacement(const FProductDefinition& Definition);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void CancelPlacement();

	UFUNCTION(BlueprintPure, Category = "Placement")
	bool IsPlacing() const { return bPlacing; }

	UFUNCTION(BlueprintPure, Category = "Placement")
	AFurnitureProduct* GetSelectedProduct() const { return SelectedProduct.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SelectProduct(AFurnitureProduct* Product);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void ClearSelection();

	/** The menu sets this while it is open so a menu gesture never also selects a room object. */
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SetUiOwnsInput(bool bInUiOwnsInput);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	UStaticMeshComponent* AreaMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	UTextRenderComponent* HintText;

private:
	void UpdatePinch();
	void UpdatePlacing();
	void UpdateIdle();
	bool TryGetFloorHit(FVector& OutHit) const;
	bool ValidatePlacement(const FVector& Location, FText& OutReason) const;
	void ConfirmPlacement();
	void SetHint(const FText& Text, const FLinearColor& Color, const FVector& WorldLocation);
	void FaceHintToCamera();
	void SetHovered(AFurnitureProduct* Product);

	TWeakObjectPtr<AFurnitureProduct> SelectedProduct;
	TWeakObjectPtr<AFurnitureProduct> HoveredProduct;

	UPROPERTY()
	AFurnitureProduct* Ghost = nullptr;

	FProductDefinition PlacingDefinition;
	bool bPlacing = false;
	bool bGhostValid = false;
	bool bUiOwnsInput = false;

	bool bPinching = false;
	bool bPinchPressed = false;
	bool bPinchReleased = false;
	/** A pinch that was already held when placement began cannot confirm it. */
	bool bAwaitingFreshPinch = false;
	/** A pinch that began while this mode was active. */
	bool bPinchStartedInMode = false;
	bool bKeyConfirm = false;
	bool bKeyCancel = false;
};
