/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "Windmill.h"
#include "SelectionCylinderHelper.h"
#include "SelectRelayComponent.h"
#include <cmath>

AWindmill::AWindmill()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Root")));
	RootComponent = RootSceneComponent;

	SelectionCylinderHelper =
		CreateDefaultSubobject<USelectionCylinderHelper>(FName(TEXT("SelectionCylinderHelper")));

	SelectRelay = CreateDefaultSubobject<USelectRelayComponent>(FName(TEXT("SelectRelay")));

	Base = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("Base")));
	Base->SetupAttachment(RootComponent);

	CollidableInteractAnchor = CreateDefaultSubobject<USceneComponent>(
		FName(TEXT("CollidableInteractAnchor")));
	CollidableInteractAnchor->SetupAttachment(RootSceneComponent);
	CollidableInteractAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 21.0f));

	SelectionMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("SelectionMesh")));
	SelectionMesh->SetupAttachment(Base);

	BladesMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("BladesMesh")));
	BladesMesh->SetupAttachment(Base);

	MaxBladesSpeed = 300.0f;
	CurrentSpeed = 0.0f;
	IsMoving = false;
	OriginalRotation = BladesMesh->GetRelativeRotation().Quaternion();
}

void AWindmill::BeginPlay()
{
	Super::BeginPlay();

	SelectRelay->OnSelected.AddUniqueDynamic(this, &AWindmill::HandleSelected);
	SelectionCylinderHelper->Initialize(SelectionMesh);
}

void AWindmill::HandleSelected(FName /*Method*/)
{
	NewMoveState(!IsMoving);
}

void AWindmill::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotationAngle += CurrentSpeed * DeltaTime;
	RotationAngle = fmod(RotationAngle, 360.0f);
	BladesMesh->SetRelativeRotation(OriginalRotation * FRotator(0, RotationAngle, 0).Quaternion());

	// Same look as before: pointed at shows the cylinder, pinching brightens it
	SelectionCylinderHelper->SetSelectionState(
		SelectRelay->IsSelected() ? ESelectionState::Highlighted
		: SelectRelay->IsHovered() ? ESelectionState::Selected
		: ESelectionState::Off);
}
