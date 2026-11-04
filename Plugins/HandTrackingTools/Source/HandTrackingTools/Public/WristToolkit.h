/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OculusXRInputFunctionLibrary.h"
#include "WristToolkit.generated.h"

class UMaterialInstanceDynamic;
class UOculusXRHandComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** The five primary items of the wrist toolkit. */
UENUM(BlueprintType)
enum class EWristToolkitItem : uint8
{
	/** Center: the host's menu (for example a catalog, undo, redo and settings) */
	Menu,
	/** Top left: move the selected object */
	Move,
	/** Top right: rotate the selected object */
	Rotate,
	/** Bottom left: choose a teleport destination */
	Navigation,
	/** Bottom right: adjust an allowed dimension of the selected object */
	ScaleDimensions,
	Count UMETA(Hidden)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWristToolkitItemActivated, EWristToolkitItem, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWristToolkitSelectionChanged, EWristToolkitItem, Item);

/**
 * A simple glove-like plate mounted on the back of one hand. It shows the five toolkit
 * items (Menu in the center and Move, Rotate, Navigation, Scale / Dimensions in the
 * corners) and activates an item when the index fingertip of the other hand pokes it.
 *
 * Place one in the level; it finds the first player pawn's OculusXRHandComponents itself.
 */
UCLASS()
class HANDTRACKINGTOOLS_API AWristToolkit : public AActor
{
	GENERATED_BODY()

public:
	AWristToolkit();

	virtual void Tick(float DeltaTime) override;

	/** Hand the toolkit is mounted on. The other hand does the poking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit")
	EOculusXRHandType MountedHand = EOculusXRHandType::HandLeft;

	/** Offset from the wrist bone, in the wrist bone's space (cm). Tune on device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Placement")
	FVector ToolkitOffset = FVector(8.0f, 0.0f, 3.0f);

	/** Rotation from the wrist bone's space. Tune on device so the plate lies on the back of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Placement")
	FRotator ToolkitRotation = FRotator::ZeroRotator;

	/** Flip the layout sideways when mounted on the right hand, since bone frames are mirrored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Placement")
	bool bMirrorForRightHand = true;

	/** Distance between the center and each corner button along both plate axes (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Layout")
	float ButtonSpacing = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Layout")
	float ButtonRadius = 1.0f;

	/** Height of the button tops above the plate (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Layout")
	float ButtonHeight = 0.6f;

	/** Fingertip must be within this height above a button top to highlight it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Interaction")
	float HoverHeight = 2.0f;

	/** Fingertip must be pushed this far below the button top to cancel a press (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Interaction")
	float PokeDepth = 1.5f;

	/** Minimum time between two activations of the same item (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Interaction")
	float ActivationCooldown = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Visuals")
	FLinearColor PlateColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Visuals")
	FLinearColor ButtonColor = FLinearColor(0.1f, 0.35f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Visuals")
	FLinearColor HoverColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Toolkit|Visuals")
	FLinearColor SelectedColor = FLinearColor(0.1f, 0.8f, 0.3f, 1.0f);

	/** Broadcast when a toolkit item is poked. Menu is momentary; the four tools also become the selected tool. */
	UPROPERTY(BlueprintAssignable, Category = "Wrist Toolkit")
	FWristToolkitItemActivated OnItemActivated;

	/** Broadcast when the selected tool (Move, Rotate, Navigation or Scale / Dimensions) changes. */
	UPROPERTY(BlueprintAssignable, Category = "Wrist Toolkit")
	FWristToolkitSelectionChanged OnSelectedToolChanged;

	/** Mount the toolkit on the other hand (the Menu's hand preference setting). */
	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	void SetMountedHand(EOculusXRHandType NewMountedHand);

	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	EWristToolkitItem GetSelectedTool() const { return SelectedTool; }

	/** Select a tool (Move, Rotate, Navigation or Scale / Dimensions). Menu is ignored. */
	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	void SetSelectedTool(EWristToolkitItem NewTool);

	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	void ClearSelectedTool();

	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	bool HasSelectedTool() const { return bHasSelectedTool; }

	/** Simulate a poke, for testing in the editor or from Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Wrist Toolkit")
	void ActivateItem(EWristToolkitItem Item);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Toolkit")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Toolkit")
	UStaticMeshComponent* Plate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Toolkit")
	TArray<UStaticMeshComponent*> ButtonMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Toolkit")
	TArray<UTextRenderComponent*> ButtonLabels;

private:
	static const int32 NumItems = static_cast<int32>(EWristToolkitItem::Count);

	/** Plate-space (x forward toward the fingers, y right, z out of the back of the hand) center of each item. */
	FVector GetItemLocalLocation(EWristToolkitItem Item) const;

	void LayoutItems();
	void FindHands();
	UOculusXRHandComponent* GetHand(EOculusXRHandType Type) const;
	bool GetBoneTransform(UOculusXRHandComponent* Hand, EOculusXRBone Bone, FTransform& OutTransform) const;
	void UpdateMount();
	void UpdatePoke(float DeltaTime);
	void UpdateButtonColors();

	UPROPERTY()
	UOculusXRHandComponent* LeftHandComponent = nullptr;

	UPROPERTY()
	UOculusXRHandComponent* RightHandComponent = nullptr;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ButtonMaterials;

	UPROPERTY()
	UMaterialInstanceDynamic* PlateMaterial = nullptr;

	EWristToolkitItem SelectedTool = EWristToolkitItem::Move;
	bool bHasSelectedTool = false;

	int32 HoveredItem = -1;
	bool bTipWasAbove[NumItems] = {};
	float TimeSinceActivation[NumItems] = {};
};
