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
#include "WristToolkit.h"
#include "WristMenu.generated.h"

class AProductPlacer;
class UMaterialInstanceDynamic;
class UProductHistorySubsystem;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EWristMenuPage : uint8
{
	Products,
	Selected,
	Settings
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWristMenuProductChosen, const FProductDefinition&, Product);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWristMenuOpenChanged, bool, bIsOpen);

/**
 * The toolkit's center Menu: an expanded panel anchored above the wrist toolkit. Row one holds Undo,
 * Redo and Close, row two switches between Products, Selected (contextual Remove with confirmation)
 * and Settings (hand preference, step size). The opposite index finger pokes it.
 *
 * Choosing a product starts a placement preview in the AProductPlacer; the menu then closes.
 *
 * Editor fallback without a headset: M = open or close, Z = undo, Y = redo, 1-5 = choose a product
 * while the Products page is showing. Without a tracked wrist the panel floats in front of the camera.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AWristMenu : public AActor
{
	GENERATED_BODY()

public:
	AWristMenu();

	virtual void Tick(float DeltaTime) override;

	/** Catalog supplied by the host. Leave empty to use the five sample furniture products. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu")
	UProductCatalogAsset* Catalog = nullptr;

	/** How far above the toolkit the panel floats, along the toolkit's up axis (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float AnchorHeight = 16.0f;

	/** Distance in front of the camera when there is no tracked wrist to anchor on (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float FallbackDistance = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float ButtonSpacing = 8.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float ButtonRadius = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float ButtonHeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Layout")
	float LabelSize = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Interaction")
	float HoverDistance = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Interaction")
	float PokeDepth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Interaction")
	float PressCooldown = 0.5f;

	/** Grid step choices the Settings page cycles through (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Interaction")
	TArray<float> StepChoicesCm = {5.0f, 10.0f, 25.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Visuals")
	FLinearColor PanelColor = FLinearColor(0.03f, 0.03f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Visuals")
	FLinearColor ButtonColor = FLinearColor(0.1f, 0.35f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Visuals")
	FLinearColor HoverColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Visuals")
	FLinearColor ActiveColor = FLinearColor(0.1f, 0.8f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrist Menu|Visuals")
	FLinearColor DisabledColor = FLinearColor(0.12f, 0.12f, 0.14f, 1.0f);

	/** Broadcast when a catalog product is chosen, just before the placement preview starts. */
	UPROPERTY(BlueprintAssignable, Category = "Wrist Menu")
	FWristMenuProductChosen OnProductChosen;

	UPROPERTY(BlueprintAssignable, Category = "Wrist Menu")
	FWristMenuOpenChanged OnOpenChanged;

	UFUNCTION(BlueprintCallable, Category = "Wrist Menu")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category = "Wrist Menu")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category = "Wrist Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintPure, Category = "Wrist Menu")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category = "Wrist Menu")
	void SetPage(EWristMenuPage NewPage);

	/** Press a button by its slot number (0-11), as a poke would. For tests and Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Wrist Menu")
	void PressSlot(int32 SlotIndex);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	UStaticMeshComponent* Backboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	TArray<UStaticMeshComponent*> ButtonMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	TArray<UTextRenderComponent*> ButtonLabels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	UTextRenderComponent* TitleText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wrist Menu")
	UTextRenderComponent* StatusText;

private:
	static const int32 Columns = 3;
	static const int32 Rows = 4;
	static const int32 NumSlots = Columns * Rows;

	enum class EMenuAction : uint8
	{
		None,
		Undo,
		Redo,
		Close,
		PageProducts,
		PageSelected,
		PageSettings,
		ChooseProduct,
		Remove,
		ConfirmRemove,
		CancelRemove,
		SwapHand,
		CycleStep
	};

	struct FMenuSlot
	{
		FText Label;
		EMenuAction Action = EMenuAction::None;
		int32 Param = 0;
		bool bVisible = false;
		bool bEnabled = true;
		bool bActive = false;
		/** Why a disabled button is unavailable; shown when it is poked. */
		FText DisabledReason;
	};

	FVector GetSlotLocalLocation(int32 SlotIndex) const;
	void LayoutPanel();
	void RebuildSlots();
	void RefreshVisuals();
	void ActivateSlot(int32 SlotIndex);
	void ShowMessage(const FText& Message);

	void FindCollaborators();
	void UpdateAnchor();
	void UpdatePoke(float DeltaTime);
	void UpdateLabelFacing();
	void HandleDebugKeys();

	UFUNCTION()
	void HandleToolkitItem(EWristToolkitItem Item);
	UFUNCTION()
	void HandleSelectionChanged(AFurnitureProduct* SelectedProduct);
	UFUNCTION()
	void HandleHistoryChanged();
	UFUNCTION()
	void HandlePlacingChanged(bool bIsPlacing);

	EOculusXRHandType GetPokeHand() const;
	const TArray<FProductDefinition>& GetProducts() const;

	UPROPERTY()
	AWristToolkit* Toolkit = nullptr;

	UPROPERTY()
	AProductPlacer* Placer = nullptr;

	UPROPERTY()
	UProductHistorySubsystem* History = nullptr;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ButtonMaterials;

	UPROPERTY()
	UMaterialInstanceDynamic* BackboardMaterial = nullptr;

	TArray<FProductDefinition> DefaultProducts;
	TArray<FMenuSlot> Slots;

	EWristMenuPage Page = EWristMenuPage::Products;
	bool bOpen = false;
	bool bConfirmingRemove = false;
	bool bBoundToolkit = false;
	int32 StepIndex = 0;
	int32 HoveredSlot = -1;
	FText Message;
	float MessageTimeLeft = 0.0f;
	float TimeSincePressed[NumSlots] = {};
	bool bTipWasInFront[NumSlots] = {};
};
