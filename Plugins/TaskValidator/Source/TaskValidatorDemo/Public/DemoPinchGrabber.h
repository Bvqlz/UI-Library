#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "DemoPinchGrabber.generated.h"

/**
 * PLACEHOLDER interaction for demoing the validator only. The real interaction methods
 * come from the UI library. Pinch (thumb tip to index tip) near an actor tagged
 * GrabbableTag to pick it up; open the pinch to drop it.
 *
 * Shows the one integration point the validator needs: on every grab it calls
 * UTaskValidatorSubsystem::ReportInteraction().
 */
UCLASS()
class TASKVALIDATORDEMO_API ADemoPinchGrabber : public AActor
{
	GENERATED_BODY()

public:
	ADemoPinchGrabber();

	/** Thumb-to-index distance (cm) that starts a pinch. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	float PinchStartDistance = 2.0f;

	/** Distance (cm) that ends it. Larger than start so the grab doesn't flicker. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	float PinchReleaseDistance = 4.0f;

	/** How close (cm) the pinch point must be to the object's bounds. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	float GrabRadius = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Demo")
	FName GrabbableTag = TEXT("Grabbable");

	/** Draw a small sphere at each pinch point (red open, green pinching). */
	UPROPERTY(EditAnywhere, Category = "Demo")
	bool bDrawPinchPoints = true;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FHandGrab
	{
		bool bPinching = false;
		TWeakObjectPtr<AActor> Held;
		FTransform HeldOffset;
		bool bHeldWasSimulating = false;
		FVector LastPinchLocation = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
	};

	void UpdateHand(EControllerHand Hand, FHandGrab& Grab, float DeltaSeconds);
	AActor* FindGrabbable(const FVector& PinchLocation) const;
	void GrabActor(FHandGrab& Grab, AActor* Actor, const FTransform& PinchTransform);
	void ReleaseHeld(FHandGrab& Grab);

	FHandGrab Hands[2];
};
