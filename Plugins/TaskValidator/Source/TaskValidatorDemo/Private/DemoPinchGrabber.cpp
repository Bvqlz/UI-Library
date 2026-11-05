#include "DemoPinchGrabber.h"

#include "Components/BillboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "HeadMountedDisplayTypes.h"
#include "TaskValidatorSubsystem.h"

ADemoPinchGrabber::ADemoPinchGrabber()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
}

void ADemoPinchGrabber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHand(EControllerHand::Left, Hands[0], DeltaSeconds);
	UpdateHand(EControllerHand::Right, Hands[1], DeltaSeconds);
}

void ADemoPinchGrabber::UpdateHand(EControllerHand Hand, FHandGrab& Grab, float DeltaSeconds)
{
	FXRHandTrackingState State;
	UHeadMountedDisplayFunctionLibrary::GetHandTrackingState(this, EXRSpaceType::UnrealWorldSpace, Hand, State);

	const bool bTracked = State.bValid
		&& State.TrackingStatus != ETrackingStatus::NotTracked
		&& State.HandKeyLocations.Num() >= EHandKeypointCount
		&& State.HandKeyRotations.Num() >= EHandKeypointCount;
	if (!bTracked)
	{
		ReleaseHeld(Grab);
		Grab.bPinching = false;
		return;
	}

	const FVector Thumb = State.HandKeyLocations[static_cast<int32>(EHandKeypoint::ThumbTip)];
	const FVector Index = State.HandKeyLocations[static_cast<int32>(EHandKeypoint::IndexTip)];
	const float PinchDistance = FVector::Dist(Thumb, Index);
	const FTransform PinchTransform(State.HandKeyRotations[static_cast<int32>(EHandKeypoint::Palm)], (Thumb + Index) * 0.5f);

	if (DeltaSeconds > 0.f)
	{
		Grab.Velocity = (PinchTransform.GetLocation() - Grab.LastPinchLocation) / DeltaSeconds;
	}
	Grab.LastPinchLocation = PinchTransform.GetLocation();

	if (!Grab.bPinching && PinchDistance < PinchStartDistance)
	{
		Grab.bPinching = true;
		if (AActor* Target = FindGrabbable(PinchTransform.GetLocation()))
		{
			GrabActor(Grab, Target, PinchTransform);
		}
	}
	else if (Grab.bPinching && PinchDistance > PinchReleaseDistance)
	{
		Grab.bPinching = false;
		ReleaseHeld(Grab);
	}

	if (AActor* Held = Grab.Held.Get())
	{
		Held->SetActorTransform(Grab.HeldOffset * PinchTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (bDrawPinchPoints)
	{
		DrawDebugSphere(GetWorld(), PinchTransform.GetLocation(), 0.8f, 8, Grab.bPinching ? FColor::Green : FColor::Red);
	}
}

AActor* ADemoPinchGrabber::FindGrabbable(const FVector& PinchLocation) const
{
	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(GrabRadius);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(GrabbableTag))
		{
			continue;
		}
		const float DistSq = It->GetComponentsBoundingBox().ComputeSquaredDistanceToPoint(PinchLocation);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	return Best;
}

void ADemoPinchGrabber::GrabActor(FHandGrab& Grab, AActor* Actor, const FTransform& PinchTransform)
{
	// Steal from the other hand if it's already holding this actor.
	for (FHandGrab& Other : Hands)
	{
		if (&Other != &Grab && Other.Held.Get() == Actor)
		{
			ReleaseHeld(Other);
		}
	}

	Grab.Held = Actor;
	Grab.HeldOffset = Actor->GetActorTransform().GetRelativeTransform(PinchTransform);
	Grab.bHeldWasSimulating = false;
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		Grab.bHeldWasSimulating = Prim->IsSimulatingPhysics();
		Prim->SetSimulatePhysics(false);
	}

	// The only line the validator needs from an interaction system.
	UTaskValidatorSubsystem::ReportInteraction(this, Actor);
}

void ADemoPinchGrabber::ReleaseHeld(FHandGrab& Grab)
{
	AActor* Held = Grab.Held.Get();
	Grab.Held.Reset();
	if (!Held || !Grab.bHeldWasSimulating)
	{
		return;
	}
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Held->GetRootComponent()))
	{
		Prim->SetSimulatePhysics(true);
		Prim->SetPhysicsLinearVelocity(Grab.Velocity);
	}
}
