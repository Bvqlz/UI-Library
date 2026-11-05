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
#include "WristToolkit.h"
#include "ProductEditTools.generated.h"

class AProductPlacer;
class UMaterialInstanceDynamic;
class UProductHistorySubsystem;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The behavior behind the four tool items of the wrist toolkit. It watches which tool the toolkit has
 * selected and turns the placer's ray and pinch into edits:
 *
 *  - Move: pinch the selected product and drag it across the floor. It snaps to the placement grid and
 *    must stay inside the placement area without overlapping another product.
 *  - Rotate: pinch the selected product and swing the ray around it. Yaw snaps to RotationStepDeg.
 *  - Scale / Dimensions: pinch the selected product and drag along its width. Only products whose
 *    capabilities allow it (bResizable) change, and only within their width range.
 *  - Navigation: point at the floor and pinch and release to teleport the player there.
 *
 * While dragging, the product is shown green (valid) or red (invalid, with the reason). Releasing a valid
 * pose commits one edit through UProductHistorySubsystem, so Undo and Redo cover it. Releasing an invalid
 * pose, losing hand tracking, opening the menu or changing the selection puts the product back.
 * Navigation is not a product edit and never enters the history.
 *
 * Place one in the level next to the AWristToolkit and AProductPlacer; it finds them itself.
 *
 * Editor fallback without a headset: G / R / N / B = Move / Rotate / Navigation / Scale (press again to put
 * the tool away). Space starts a drag, and Space again commits it; Escape cancels. In Navigation, Space teleports.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AProductEditTools : public AActor
{
	GENERATED_BODY()

public:
	AProductEditTools();

	virtual void Tick(float DeltaTime) override;

	/** Rotation snaps to this many degrees. Zero turns snapping off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	float RotationStepDeg = 15.0f;

	/** Navigation only accepts destinations this close to the player (cm, measured along the floor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	float MaxTeleportDistanceCm = 600.0f;

	/** Navigation keeps this much room between the player and any product (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	float PlayerClearanceCm = 25.0f;

	/** True while a Move, Rotate or Scale drag is in progress. */
	UFUNCTION(BlueprintPure, Category = "Tools")
	bool IsDragging() const { return bDragging; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools")
	UTextRenderComponent* HintText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	UStaticMeshComponent* DestinationMarker;

private:
	void FindCollaborators();
	void HandleEditorKeys();

	/** Back to idle: no drag, no pinch ownership, nothing shown. */
	void Deactivate();
	void EnterTool(EWristToolkitItem Tool);

	void TickProductTool(EWristToolkitItem Tool);
	void TickNavigation();

	bool RayHitsProduct(const AFurnitureProduct* Product) const;
	bool CanUseTool(EWristToolkitItem Tool, const AFurnitureProduct* Product, FText& OutReason) const;
	void BeginDrag(EWristToolkitItem Tool, AFurnitureProduct* Product, const FVector& FloorHit, bool bKeyboard);
	void UpdateDrag();
	void ApplyDragPose(const FVector& FloorHit);
	void FinishDrag(bool bCommit);
	void RestoreStartState(AFurnitureProduct* Product);

	bool ValidateTeleport(const FVector& Target, FText& OutReason) const;
	void Teleport(const FVector& Target);

	void ShowMessage(const FText& Text, const FLinearColor& Color, float DurationSeconds = 2.0f);
	void SetHint(const FText& Text, const FLinearColor& Color, const FVector& WorldLocation);
	void HideHint();
	FVector InFrontOfCamera(float DistanceCm) const;
	void FaceHintToCamera();

	UPROPERTY()
	AWristToolkit* Toolkit = nullptr;

	UPROPERTY()
	AProductPlacer* Placer = nullptr;

	UPROPERTY()
	UProductHistorySubsystem* History = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MarkerMaterial = nullptr;

	float CollaboratorSearchTimer = 0.0f;

	bool bHasActiveTool = false;
	EWristToolkitItem ActiveTool = EWristToolkitItem::Move;

	// Drag state (Move, Rotate, Scale)
	bool bDragging = false;
	bool bKeyboardDrag = false;
	bool bDragValid = true;
	EWristToolkitItem DragTool = EWristToolkitItem::Move;
	TWeakObjectPtr<AFurnitureProduct> DragProduct;
	FTransform DragStartTransform;
	FProductDefinition DragStartDefinition;
	FVector DragGrabOffset = FVector::ZeroVector;
	float DragStartAngleDeg = 0.0f;
	float DragStartYawDeg = 0.0f;
	FVector DragWidthAxis = FVector::RightVector;
	float DragStartProjection = 0.0f;
	float DragWidthSign = 1.0f;
	FText DragReason;
	FString DragLabel;

	// Navigation state
	bool bNavPinchStarted = false;

	// A timed message shown instead of the idle hint
	FText MessageText;
	FLinearColor MessageColor = FLinearColor::White;
	float MessageTimeRemaining = 0.0f;
};
