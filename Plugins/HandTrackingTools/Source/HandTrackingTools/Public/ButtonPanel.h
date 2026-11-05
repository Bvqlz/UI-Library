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
#include "ButtonPanel.generated.h"

class UMaterialInstanceDynamic;
class UOculusXRHandComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonPanelButtonPressed, int32, ButtonIndex, const FText&, Label);

/**
 * A panel of 8 poke buttons (4 columns x 2 rows) with editable labels. Either index fingertip
 * presses a button. Place one in the level.
 *
 * Panel space: the face points along local +X, local +Z is up. Seen from the front the columns
 * run along local -Y, so a panel facing the player is placed with a yaw of 180 degrees.
 */
UCLASS()
class HANDTRACKINGTOOLS_API AButtonPanel : public AActor
{
	GENERATED_BODY()

public:
	AButtonPanel();

	virtual void Tick(float DeltaTime) override;

	/** Button labels, left to right, top row first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel")
	TArray<FText> Labels;

	/** Distance between neighbouring button centers (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Layout")
	float ButtonSpacing = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Layout")
	float ButtonRadius = 3.0f;

	/** Height of the button faces above the panel face (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Layout")
	float ButtonHeight = 1.5f;

	/** Fingertip must be within this distance in front of a button face to highlight it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Interaction")
	float HoverDistance = 3.0f;

	/** Fingertip pushed this far behind a button face no longer counts as a press (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Interaction")
	float PokeDepth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Interaction")
	float PressCooldown = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Visuals")
	FLinearColor PanelColor = FLinearColor(0.35f, 0.22f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Visuals")
	FLinearColor ButtonColor = FLinearColor(0.7f, 0.15f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Visuals")
	FLinearColor HoverColor = FLinearColor(1.0f, 0.5f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Panel|Visuals")
	FLinearColor PressedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

	/** Broadcast when a button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "Button Panel")
	FButtonPanelButtonPressed OnButtonPressed;

	UFUNCTION(BlueprintCallable, Category = "Button Panel")
	void SetLabel(int32 ButtonIndex, const FText& NewLabel);

	/** Simulate a press, for testing in the editor or from Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Button Panel")
	void PressButton(int32 ButtonIndex);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Button Panel")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Button Panel")
	UStaticMeshComponent* Backboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Button Panel")
	TArray<UStaticMeshComponent*> ButtonMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Button Panel")
	TArray<UTextRenderComponent*> ButtonLabels;

private:
	static const int32 Columns = 4;
	static const int32 Rows = 2;
	static const int32 NumButtons = Columns * Rows;

	/** Panel-space center of a button on the panel face. */
	FVector GetButtonLocalLocation(int32 ButtonIndex) const;

	void LayoutButtons();
	void RefreshLabels();
	void FindHands();
	bool GetIndexTip(UOculusXRHandComponent* Hand, EOculusXRHandType Type, FVector& OutWorldLocation) const;
	void UpdatePoke(float DeltaTime);
	void UpdateLabelFacing();
	void UpdateButtonColors();

	UPROPERTY()
	UOculusXRHandComponent* LeftHandComponent = nullptr;

	UPROPERTY()
	UOculusXRHandComponent* RightHandComponent = nullptr;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ButtonMaterials;

	UPROPERTY()
	UMaterialInstanceDynamic* BackboardMaterial = nullptr;

	int32 HoveredButton = -1;
	int32 PressedButton = -1;
	bool bTipWasInFront[NumButtons] = {};
	float TimeSincePressed[NumButtons] = {};
};
