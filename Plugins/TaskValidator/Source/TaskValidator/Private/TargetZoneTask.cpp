#include "TargetZoneTask.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"

namespace
{
	const FColor IdleColor(128, 128, 128);
	const FColor ActiveColor(255, 200, 0);
	const FColor InsideColor(0, 220, 80);
}

ATargetZoneTask::ATargetZoneTask()
{
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	Zone->SetBoxExtent(FVector(10.f));
	Zone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Zone->SetGenerateOverlapEvents(false);
	Zone->SetCanEverAffectNavigation(false);
	Zone->SetLineThickness(0.5f);
	Zone->ShapeColor = IdleColor;
	RootComponent = Zone;
}

void ATargetZoneTask::BeginPlay()
{
	Super::BeginPlay();
	Zone->SetHiddenInGame(!bShowZoneInGame);
	if (TargetActor)
	{
		TargetStartTransform = TargetActor->GetActorTransform();
	}
}

void ATargetZoneTask::OnTaskBegin()
{
	DwellTimer = 0.f;
	SetZoneColor(ActiveColor);
	if (!TargetActor)
	{
		return;
	}

	if (bResetTargetOnStart)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
		{
			Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		TargetActor->SetActorTransform(TargetStartTransform, false, nullptr, ETeleportType::ResetPhysics);
	}
	LastTargetLocation = TargetActor->GetActorLocation();
}

void ATargetZoneTask::TickActiveTask(float DeltaSeconds)
{
	if (!TargetActor || DeltaSeconds <= 0.f)
	{
		return;
	}

	const FVector Location = TargetActor->GetActorLocation();
	const float Speed = FVector::Dist(Location, LastTargetLocation) / DeltaSeconds;
	LastTargetLocation = Location;

	float RotationError = 0.f;
	const bool bPlaced = IsTargetPlaced(RotationError);
	SetZoneColor(bPlaced ? InsideColor : ActiveColor);

	DwellTimer = (bPlaced && Speed <= MaxSettledSpeed) ? DwellTimer + DeltaSeconds : 0.f;
	if (DwellTimer >= DwellSeconds)
	{
		MarkConditionMet();
	}
}

void ATargetZoneTask::OnTaskEnd(EValidatorTaskResult Result, FValidatorTaskRecord& InOutRecord)
{
	SetZoneColor(IdleColor);
	if (TargetActor)
	{
		InOutRecord.PositionErrorCm = FVector::Dist(TargetActor->GetActorLocation(), Zone->GetComponentLocation());
		InOutRecord.RotationErrorDeg = RotationCheck == EZoneRotationCheck::Ignore ? -1.f : ComputeRotationError();
	}
}

bool ATargetZoneTask::IsTargetPlaced(float& OutRotationErrorDeg) const
{
	// Inverse transform includes the zone's scale, so compare against the unscaled extent.
	const FVector Local = Zone->GetComponentTransform().InverseTransformPosition(TargetActor->GetActorLocation());
	const FVector Extent = Zone->GetUnscaledBoxExtent();
	if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y || FMath::Abs(Local.Z) > Extent.Z)
	{
		return false;
	}

	if (RotationCheck == EZoneRotationCheck::Ignore)
	{
		OutRotationErrorDeg = 0.f;
		return true;
	}
	OutRotationErrorDeg = ComputeRotationError();
	return OutRotationErrorDeg <= RotationToleranceDegrees;
}

float ATargetZoneTask::ComputeRotationError() const
{
	const FQuat TargetRotation = TargetActor->GetActorQuat();
	const FQuat ZoneRotation = Zone->GetComponentQuat();
	if (RotationCheck == EZoneRotationCheck::UprightOnly)
	{
		const float Dot = FVector::DotProduct(TargetRotation.GetUpVector(), ZoneRotation.GetUpVector());
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	}
	return FMath::RadiansToDegrees(TargetRotation.AngularDistance(ZoneRotation));
}

void ATargetZoneTask::SetZoneColor(const FColor& Color)
{
	if (Zone->ShapeColor != Color)
	{
		Zone->ShapeColor = Color;
		Zone->MarkRenderStateDirty();
	}
}
