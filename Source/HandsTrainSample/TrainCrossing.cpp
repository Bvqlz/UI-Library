/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "TrainCrossing.h"
#include "SelectionCylinderHelper.h"
#include "SelectRelayComponent.h"
#include "Components/AudioComponent.h"

ATrainCrossing::ATrainCrossing()
{
	PrimaryActorTick.bCanEverTick = true;
	SelectRelay = CreateDefaultSubobject<USelectRelayComponent>(FName(TEXT("SelectRelay")));
}

void ATrainCrossing::BeginPlay()
{
	Super::BeginPlay();

	RRXingMeshComp = (UStaticMeshComponent*)GetDefaultSubobjectByName(FName(TEXT("RRXing")));
	SelectionMeshComp = (UStaticMeshComponent*)GetDefaultSubobjectByName(FName(TEXT("SelectionMesh")));
	Light1Mesh = (UStaticMeshComponent*)GetDefaultSubobjectByName(FName(TEXT("LightSurface1")));
	Light2Mesh = (UStaticMeshComponent*)GetDefaultSubobjectByName(FName(TEXT("LightSurface2")));
	AudioComp = (UAudioComponent*)GetDefaultSubobjectByName(FName(TEXT("XingAudio")));
	CollidableAnchor = (USceneComponent*)GetDefaultSubobjectByName(
		FName(TEXT("CollidableInteractAnchor")));
	SelectionCylinderHelperComp = (USelectionCylinderHelper*)GetDefaultSubobjectByName(
		FName(TEXT("SelectionCylinderHelper")));

	SelectRelay->OnSelected.AddUniqueDynamic(this, &ATrainCrossing::HandleSelected);
	SelectionCylinderHelperComp->Initialize(SelectionMeshComp);
}

void ATrainCrossing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Same look as before: pointed at shows the cylinder, pinching brightens it
	SelectionCylinderHelperComp->SetSelectionState(
		SelectRelay->IsSelected() ? ESelectionState::Highlighted
		: SelectRelay->IsHovered() ? ESelectionState::Selected
		: ESelectionState::Off);
}

void ATrainCrossing::HandleSelected(FName /*Method*/)
{
	ActivateTrainCrossing();
}

void ATrainCrossing::ActivateTrainCrossing()
{
	AudioComp->SetSound(CrossingSound);
	AudioComp->Activate(false);
	AudioComp->Play();

	ToggleLightObjects(true);
	float AnimationLength = CrossingSound->Duration * 0.75f;
	float lightBlinkDuration = AnimationLength * 0.1f;
	BlinkFirstLight = true;
	BlinkLights(lightBlinkDuration, lightBlinkDuration, AnimationLength);
}

void ATrainCrossing::ToggleLightObjects(bool enableState)
{
	Light1Mesh->SetVisibility(enableState, true);
	Light2Mesh->SetVisibility(enableState, true);
}
