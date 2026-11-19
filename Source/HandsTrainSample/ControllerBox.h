/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsdkRoundedButton.h"
#include "ControllerBox.generated.h"

class ATrainLocomotive;
class ACowCar;
class UStaticMeshComponent;
class AHandsVisualizationSwitcher;

/** The eight controller box buttons. */
UENUM(BlueprintType)
enum class EControllerBoxButton : uint8
{
	Smoke,
	Whistle,
	MooCow,
	HandStyle,
	Reverse,
	SpeedUp,
	SlowDown,
	StartStop,
	Count UMETA(Hidden)
};

/**
 * Manages the controller box buttons. This was created from an FBX
 * via "File->Import Into Level..." because the scene graph of
 * ControllerBox.fbx must be respected (especially in relation to
 * button placement). The resulting blueprint created from the import
 * process was modified to accommodate this class.
 *
 * The buttons are Meta Interaction SDK rounded poke buttons (UIsdkRoundedButtonComponent), one per
 * EControllerBoxButton, created at BeginPlay on the "<Name>ButtonAnchor" components of the Blueprint.
 * A poke Select on a button runs its action. The validator bridge counts the same Select as an attempt
 * and reports this ControllerBox, because the buttons are its own components.
 */
UCLASS()
class HANDSTRAINSAMPLE_API AControllerBox : public AActor
{
	GENERATED_BODY()

public:
	AControllerBox();

	virtual void Tick(float DeltaTime) override;

	/** Runs a button's action, the same as poking it. */
	UFUNCTION(BlueprintCallable, Category = "Button Events")
	void PressButton(EControllerBoxButton Button);

	/** The Interaction SDK button for an action, or null before the buttons exist. */
	UIsdkRoundedButtonComponent* GetButton(EControllerBoxButton Button) const;

protected:
	virtual void BeginPlay() override;

	/** Button face size in cm. */
	UPROPERTY(EditAnywhere, Category = "Buttons")
	FVector2D ButtonSize = FVector2D(4.0, 4.0);

	UPROPERTY(EditAnywhere, Category = "Buttons")
	float ButtonLabelSize = 0.8f;

	/**
	 * Button rotation relative to its anchor. The anchors come from the FBX and were oriented for the
	 * old buttons; adjust this if the Interaction SDK buttons don't face the user.
	 */
	UPROPERTY(EditAnywhere, Category = "Buttons")
	FRotator ButtonRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distances")
	float MaxDistanceFromHMDToLastPos;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distances")
	float MinDistanceToHMD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distances")
	float MinDepthToHMD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Positioning")
	FVector PanelOffsetFromHMD;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positioning")
	FVector LastMovedToPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positioning")
	FVector PrevPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Lerping")
	bool bIsLerpingToHMD;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ATrainLocomotive* Locomotive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ACowCar* CowCar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AHandsVisualizationSwitcher* HandsVisSwitcher;

	UFUNCTION(BlueprintCallable)
	void FindLocomotive();

	UFUNCTION(BlueprintCallable)
	void FindCowCar();

	UFUNCTION(BlueprintCallable)
	void FindHandsVisualizationSwitcher();

	UFUNCTION(BlueprintCallable)
	FVector GetHMDPosition();

	UFUNCTION(BlueprintCallable)
	FVector GetIdealAnchorPosition();

	// This is a blueprint event because it's easy to do delays in blueprints.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,
		Category = "Animation")
	void LerpToPosition(float LerpTotalDuration, float TimeLeftForLerp,
		FVector StartPosition, FVector EndPosition);

private:
	const static float TotalFollowDuration;
	const static float HmdMovementThreshold;

	void CreateButtons();
	void HookUpButtonEvents();

	UFUNCTION()
	void HandleButtonPointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);

	/** Indexed by EControllerBoxButton. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIsdkRoundedButtonComponent>> Buttons;

	void StartStopTrain();
	void TrainBlowSmoke();
	void CowMoo();
	void TrainBlowWhistle();

	void TrainSpeedUp();
	void TrainSlowDown();
	void TrainReverse();
};
