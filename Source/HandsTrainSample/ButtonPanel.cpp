/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ButtonPanel.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "OculusXRHandComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const float PanelThickness = 2.0f;	  // cm
	const float PanelMargin = 4.0f;		  // cm around the outer buttons
	const float FingertipRadius = 0.6f;	  // extra reach around the fingertip (cm)
	const float PressedFlashSeconds = 0.3f;
	const FName ColorParamName = FName(TEXT("Color"));
}

AButtonPanel::AButtonPanel()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Backboard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backboard"));
	Backboard->SetupAttachment(RootComponent);
	Backboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backboard->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		Backboard->SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Backboard->SetMaterial(0, ShapeMaterial.Object);
	}

	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		UStaticMeshComponent* ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Button_%d"), ButtonIndex));
		ButtonMesh->SetupAttachment(RootComponent);
		ButtonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ButtonMesh->SetCastShadow(false);
		if (CylinderMesh.Succeeded())
		{
			ButtonMesh->SetStaticMesh(CylinderMesh.Object);
		}
		if (ShapeMaterial.Succeeded())
		{
			ButtonMesh->SetMaterial(0, ShapeMaterial.Object);
		}
		ButtonMeshes.Add(ButtonMesh);

		UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(
			*FString::Printf(TEXT("Label_%d"), ButtonIndex));
		Label->SetupAttachment(RootComponent);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextTop);
		Label->SetWorldSize(1.6f);
		Label->SetTextRenderColor(FColor::White);
		Label->SetCastShadow(false);
		ButtonLabels.Add(Label);

		Labels.Add(FText::FromString(FString::Printf(TEXT("Button %d"), ButtonIndex + 1)));
	}

	LayoutButtons();
	RefreshLabels();
}

void AButtonPanel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutButtons();
	RefreshLabels();
}

FVector AButtonPanel::GetButtonLocalLocation(int32 ButtonIndex) const
{
	const int32 Column = ButtonIndex % Columns;
	const int32 Row = ButtonIndex / Columns;

	// Seen from the front the columns run left to right along local -Y and rows run top to bottom along -Z
	const float Y = -(Column - (Columns - 1) * 0.5f) * ButtonSpacing;
	const float Z = -(Row - (Rows - 1) * 0.5f) * ButtonSpacing;
	return FVector(0.0f, Y, Z);
}

void AButtonPanel::LayoutButtons()
{
	const float Width = (Columns - 1) * ButtonSpacing + 2.0f * (ButtonRadius + PanelMargin);
	const float Height = (Rows - 1) * ButtonSpacing + 2.0f * (ButtonRadius + PanelMargin);

	// Panel face is the x = 0 plane; the backboard sits behind it
	Backboard->SetRelativeLocation(FVector(-PanelThickness * 0.5f, 0.0f, 0.0f));
	Backboard->SetRelativeScale3D(FVector(PanelThickness, Width, Height) * 0.01f);

	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		const FVector Center = GetButtonLocalLocation(ButtonIndex);

		// The cylinder mesh stands along Z; pitch it so it stands out of the face along X
		ButtonMeshes[ButtonIndex]->SetRelativeLocation(Center + FVector(ButtonHeight * 0.5f, 0.0f, 0.0f));
		ButtonMeshes[ButtonIndex]->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		ButtonMeshes[ButtonIndex]->SetRelativeScale3D(
			FVector(ButtonRadius * 2.0f, ButtonRadius * 2.0f, ButtonHeight) * 0.01f);

		// Labels sit just below each button; UpdateLabelFacing keeps them turned toward the viewer
		ButtonLabels[ButtonIndex]->SetRelativeLocation(
			Center + FVector(ButtonHeight + 0.1f, 0.0f, -(ButtonRadius + 0.4f)));
	}
}

void AButtonPanel::RefreshLabels()
{
	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		ButtonLabels[ButtonIndex]->SetText(Labels.IsValidIndex(ButtonIndex) ? Labels[ButtonIndex] : FText::GetEmpty());
	}
}

void AButtonPanel::BeginPlay()
{
	Super::BeginPlay();

	BackboardMaterial = Backboard->CreateDynamicMaterialInstance(0);
	if (BackboardMaterial)
	{
		BackboardMaterial->SetVectorParameterValue(ColorParamName, PanelColor);
	}

	ButtonMaterials.Reset();
	for (UStaticMeshComponent* ButtonMesh : ButtonMeshes)
	{
		ButtonMaterials.Add(ButtonMesh->CreateDynamicMaterialInstance(0));
	}

	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		TimeSincePressed[ButtonIndex] = PressCooldown;
	}

	LayoutButtons();
	RefreshLabels();
	UpdateButtonColors();
	FindHands();
}

void AButtonPanel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!LeftHandComponent || !RightHandComponent)
	{
		FindHands();
	}

	UpdatePoke(DeltaTime);
	UpdateLabelFacing();
}

void AButtonPanel::FindHands()
{
	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	TArray<UOculusXRHandComponent*> HandComponents;
	PlayerPawn->GetComponents<UOculusXRHandComponent>(HandComponents);
	for (UOculusXRHandComponent* HandComponent : HandComponents)
	{
		if (HandComponent->SkeletonType == EOculusXRHandType::HandLeft)
		{
			LeftHandComponent = HandComponent;
		}
		else if (HandComponent->SkeletonType == EOculusXRHandType::HandRight)
		{
			RightHandComponent = HandComponent;
		}
	}
}

bool AButtonPanel::GetIndexTip(UOculusXRHandComponent* Hand, EOculusXRHandType Type, FVector& OutWorldLocation) const
{
	if (!IsValid(Hand) || !Hand->GetSkinnedAsset() || !UOculusXRInputFunctionLibrary::IsHandPositionValid(Type))
	{
		return false;
	}

	const FName BoneName(*UOculusXRInputFunctionLibrary::GetBoneName(EOculusXRBone::Index_Tip));
	const int32 BoneIndex = Hand->GetBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		return false;
	}

	OutWorldLocation = Hand->GetBoneTransform(BoneIndex).GetLocation();
	return true;
}

void AButtonPanel::UpdatePoke(float DeltaTime)
{
	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		TimeSincePressed[ButtonIndex] += DeltaTime;
	}

	// Collect the fingertips of both hands in panel space
	TArray<FVector> Tips;
	FVector TipWorld;
	if (GetIndexTip(LeftHandComponent, EOculusXRHandType::HandLeft, TipWorld))
	{
		Tips.Add(GetActorTransform().InverseTransformPosition(TipWorld));
	}
	if (GetIndexTip(RightHandComponent, EOculusXRHandType::HandRight, TipWorld))
	{
		Tips.Add(GetActorTransform().InverseTransformPosition(TipWorld));
	}

	int32 NewHoveredButton = -1;
	for (int32 ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex)
	{
		const FVector Center = GetButtonLocalLocation(ButtonIndex);

		bool bInFront = false;
		bool bPressing = false;
		for (const FVector& Tip : Tips)
		{
			const float SideDistance = FVector2D(Tip.Y - Center.Y, Tip.Z - Center.Z).Size();
			const float DistanceInFront = Tip.X - ButtonHeight;
			if (SideDistance > ButtonRadius + FingertipRadius)
			{
				continue;
			}
			if (DistanceInFront > 0.0f && DistanceInFront <= HoverDistance)
			{
				bInFront = true;
			}
			else if (DistanceInFront <= 0.0f && DistanceInFront >= -PokeDepth)
			{
				bPressing = true;
			}
		}

		if (bInFront || bPressing)
		{
			NewHoveredButton = ButtonIndex;
		}

		// A press is a fingertip crossing the button face from the front
		if (bPressing && bTipWasInFront[ButtonIndex] && TimeSincePressed[ButtonIndex] >= PressCooldown)
		{
			PressButton(ButtonIndex);
		}
		bTipWasInFront[ButtonIndex] = bInFront;
	}

	const bool bFlashing = PressedButton >= 0 && TimeSincePressed[PressedButton] < PressedFlashSeconds;
	if (!bFlashing && PressedButton >= 0)
	{
		PressedButton = -1;
		UpdateButtonColors();
	}

	if (NewHoveredButton != HoveredButton)
	{
		HoveredButton = NewHoveredButton;
		UpdateButtonColors();
	}
}

void AButtonPanel::UpdateLabelFacing()
{
	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	// Text components face +X; turn each toward the viewer so the text always reads correctly
	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	for (UTextRenderComponent* Label : ButtonLabels)
	{
		Label->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(Label->GetComponentLocation(), CameraLocation));
	}
}

void AButtonPanel::UpdateButtonColors()
{
	for (int32 ButtonIndex = 0; ButtonIndex < ButtonMaterials.Num(); ++ButtonIndex)
	{
		if (!ButtonMaterials[ButtonIndex])
		{
			continue;
		}

		FLinearColor Color = ButtonColor;
		if (ButtonIndex == PressedButton)
		{
			Color = PressedColor;
		}
		else if (ButtonIndex == HoveredButton)
		{
			Color = HoverColor;
		}
		ButtonMaterials[ButtonIndex]->SetVectorParameterValue(ColorParamName, Color);
	}
}

void AButtonPanel::SetLabel(int32 ButtonIndex, const FText& NewLabel)
{
	if (ButtonIndex < 0 || ButtonIndex >= NumButtons)
	{
		return;
	}

	if (!Labels.IsValidIndex(ButtonIndex))
	{
		Labels.SetNum(NumButtons);
	}
	Labels[ButtonIndex] = NewLabel;
	RefreshLabels();
}

void AButtonPanel::PressButton(int32 ButtonIndex)
{
	if (ButtonIndex < 0 || ButtonIndex >= NumButtons)
	{
		return;
	}

	TimeSincePressed[ButtonIndex] = 0.0f;
	PressedButton = ButtonIndex;
	UpdateButtonColors();
	OnButtonPressed.Broadcast(ButtonIndex, Labels.IsValidIndex(ButtonIndex) ? Labels[ButtonIndex] : FText::GetEmpty());
}
