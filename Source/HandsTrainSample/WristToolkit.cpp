/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "WristToolkit.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "OculusXRHandComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const float PlateWidth = 7.0f;	  // across the back of the hand (cm)
	const float PlateLength = 8.0f;	  // wrist toward the knuckles (cm)
	const float PlateThickness = 0.4f; // (cm)
	const float FingertipRadius = 0.5f; // extra reach around the fingertip (cm)
	const FName ColorParamName = FName(TEXT("Color"));

	const TCHAR* GetItemLabel(EWristToolkitItem Item)
	{
		switch (Item)
		{
			case EWristToolkitItem::Menu:
				return TEXT("MENU");
			case EWristToolkitItem::Move:
				return TEXT("MOVE");
			case EWristToolkitItem::Rotate:
				return TEXT("ROTATE");
			case EWristToolkitItem::Navigation:
				return TEXT("NAV");
			case EWristToolkitItem::ScaleDimensions:
				return TEXT("SCALE");
			default:
				return TEXT("");
		}
	}
}

AWristToolkit::AWristToolkit()
{
	PrimaryActorTick.bCanEverTick = true;
	// Run after the hand components have posed their bones for this frame
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(RootComponent);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		Plate->SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Plate->SetMaterial(0, ShapeMaterial.Object);
	}

	for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
	{
		const EWristToolkitItem Item = static_cast<EWristToolkitItem>(ItemIndex);

		UStaticMeshComponent* ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Button_%d"), ItemIndex));
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
			*FString::Printf(TEXT("Label_%d"), ItemIndex));
		Label->SetupAttachment(RootComponent);
		Label->SetText(FText::FromString(GetItemLabel(Item)));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextCenter);
		Label->SetWorldSize(0.45f);
		Label->SetTextRenderColor(FColor::White);
		Label->SetCastShadow(false);
		ButtonLabels.Add(Label);
	}

	LayoutItems();
}

void AWristToolkit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutItems();
}

FVector AWristToolkit::GetItemLocalLocation(EWristToolkitItem Item) const
{
	const float Side = (bMirrorForRightHand && MountedHand == EOculusXRHandType::HandRight) ? -1.0f : 1.0f;
	// x: toward the fingers ("top"), y: the user's right, z: out of the back of the hand
	switch (Item)
	{
		case EWristToolkitItem::Menu:
			return FVector(0.0f, 0.0f, 0.0f);
		case EWristToolkitItem::Move:
			return FVector(ButtonSpacing, -ButtonSpacing * Side, 0.0f);
		case EWristToolkitItem::Rotate:
			return FVector(ButtonSpacing, ButtonSpacing * Side, 0.0f);
		case EWristToolkitItem::Navigation:
			return FVector(-ButtonSpacing, -ButtonSpacing * Side, 0.0f);
		case EWristToolkitItem::ScaleDimensions:
			return FVector(-ButtonSpacing, ButtonSpacing * Side, 0.0f);
		default:
			return FVector::ZeroVector;
	}
}

void AWristToolkit::LayoutItems()
{
	// Plate-space has z = 0 at the plate's top surface
	Plate->SetRelativeLocation(FVector(0.0f, 0.0f, -PlateThickness * 0.5f));
	Plate->SetRelativeScale3D(FVector(PlateLength, PlateWidth, PlateThickness) * 0.01f);

	for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
	{
		const FVector ItemLocation = GetItemLocalLocation(static_cast<EWristToolkitItem>(ItemIndex));

		ButtonMeshes[ItemIndex]->SetRelativeLocation(ItemLocation + FVector(0.0f, 0.0f, ButtonHeight * 0.5f));
		ButtonMeshes[ItemIndex]->SetRelativeScale3D(
			FVector(ButtonRadius * 2.0f, ButtonRadius * 2.0f, ButtonHeight) * 0.01f);

		// Text faces +X by default; pitch it up so it reads on top of the button
		ButtonLabels[ItemIndex]->SetRelativeLocation(ItemLocation + FVector(0.0f, 0.0f, ButtonHeight + 0.05f));
		ButtonLabels[ItemIndex]->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
}

void AWristToolkit::BeginPlay()
{
	Super::BeginPlay();

	PlateMaterial = Plate->CreateDynamicMaterialInstance(0);
	if (PlateMaterial)
	{
		PlateMaterial->SetVectorParameterValue(ColorParamName, PlateColor);
	}

	ButtonMaterials.Reset();
	for (UStaticMeshComponent* ButtonMesh : ButtonMeshes)
	{
		ButtonMaterials.Add(ButtonMesh->CreateDynamicMaterialInstance(0));
	}

	for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
	{
		TimeSinceActivation[ItemIndex] = ActivationCooldown;
	}

	LayoutItems();
	UpdateButtonColors();
	FindHands();
}

void AWristToolkit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!LeftHandComponent || !RightHandComponent)
	{
		FindHands();
	}

	UpdateMount();
	UpdatePoke(DeltaTime);
}

void AWristToolkit::FindHands()
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

UOculusXRHandComponent* AWristToolkit::GetHand(EOculusXRHandType Type) const
{
	return Type == EOculusXRHandType::HandLeft ? LeftHandComponent : RightHandComponent;
}

bool AWristToolkit::GetBoneTransform(UOculusXRHandComponent* Hand, EOculusXRBone Bone, FTransform& OutTransform) const
{
	if (!IsValid(Hand) || !Hand->GetSkinnedAsset())
	{
		return false;
	}

	const FName BoneName(*UOculusXRInputFunctionLibrary::GetBoneName(Bone));
	const int32 BoneIndex = Hand->GetBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		return false;
	}

	OutTransform = Hand->GetBoneTransform(BoneIndex);
	return true;
}

void AWristToolkit::UpdateMount()
{
	UOculusXRHandComponent* Hand = GetHand(MountedHand);
	if (!IsValid(Hand) || !UOculusXRInputFunctionLibrary::IsHandPositionValid(MountedHand))
	{
		SetActorHiddenInGame(true);
		return;
	}

	// Anchor on the wrist bone when the hand has a skeleton, otherwise on the hand component itself
	FTransform WristTransform = Hand->GetComponentTransform();
	GetBoneTransform(Hand, EOculusXRBone::Wrist_Root, WristTransform);

	SetActorHiddenInGame(false);
	SetActorTransform(FTransform(ToolkitRotation, ToolkitOffset) * WristTransform);
}

void AWristToolkit::UpdatePoke(float DeltaTime)
{
	const EOculusXRHandType PokingHandType =
		MountedHand == EOculusXRHandType::HandLeft ? EOculusXRHandType::HandRight : EOculusXRHandType::HandLeft;

	for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
	{
		TimeSinceActivation[ItemIndex] += DeltaTime;
	}

	FTransform TipTransform;
	const bool bHasTip = !IsHidden() && UOculusXRInputFunctionLibrary::IsHandPositionValid(PokingHandType) &&
		GetBoneTransform(GetHand(PokingHandType), EOculusXRBone::Index_Tip, TipTransform);

	int32 NewHoveredItem = -1;
	if (bHasTip)
	{
		const FVector Tip = GetActorTransform().InverseTransformPosition(TipTransform.GetLocation());

		for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
		{
			const FVector Center = GetItemLocalLocation(static_cast<EWristToolkitItem>(ItemIndex));
			const float PlanarDistance = FVector2D(Tip.X - Center.X, Tip.Y - Center.Y).Size();
			const float HeightAboveTop = Tip.Z - ButtonHeight;
			const bool bOverButton = PlanarDistance <= ButtonRadius + FingertipRadius;

			const bool bAbove = bOverButton && HeightAboveTop > 0.0f && HeightAboveTop <= HoverHeight;
			const bool bPressed = bOverButton && HeightAboveTop <= 0.0f && HeightAboveTop >= -PokeDepth;

			if (bAbove || bPressed)
			{
				NewHoveredItem = ItemIndex;
			}

			// A press is a fingertip crossing the button top from above
			if (bPressed && bTipWasAbove[ItemIndex] && TimeSinceActivation[ItemIndex] >= ActivationCooldown)
			{
				ActivateItem(static_cast<EWristToolkitItem>(ItemIndex));
			}
			bTipWasAbove[ItemIndex] = bAbove;
		}
	}
	else
	{
		for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
		{
			bTipWasAbove[ItemIndex] = false;
		}
	}

	if (NewHoveredItem != HoveredItem)
	{
		HoveredItem = NewHoveredItem;
		UpdateButtonColors();
	}
}

void AWristToolkit::UpdateButtonColors()
{
	for (int32 ItemIndex = 0; ItemIndex < ButtonMaterials.Num(); ++ItemIndex)
	{
		if (!ButtonMaterials[ItemIndex])
		{
			continue;
		}

		const EWristToolkitItem Item = static_cast<EWristToolkitItem>(ItemIndex);
		const bool bIsSelectedTool = bHasSelectedTool && Item == SelectedTool && Item != EWristToolkitItem::Menu;

		FLinearColor Color = ButtonColor;
		if (ItemIndex == HoveredItem)
		{
			Color = HoverColor;
		}
		else if (bIsSelectedTool)
		{
			Color = SelectedColor;
		}
		ButtonMaterials[ItemIndex]->SetVectorParameterValue(ColorParamName, Color);
	}
}

void AWristToolkit::ActivateItem(EWristToolkitItem Item)
{
	const int32 ItemIndex = static_cast<int32>(Item);
	if (ItemIndex < 0 || ItemIndex >= NumItems)
	{
		return;
	}

	TimeSinceActivation[ItemIndex] = 0.0f;

	// Menu is momentary; the four tools become the selected tool
	if (Item != EWristToolkitItem::Menu)
	{
		SetSelectedTool(Item);
	}
	OnItemActivated.Broadcast(Item);
}

void AWristToolkit::SetSelectedTool(EWristToolkitItem NewTool)
{
	if (NewTool == EWristToolkitItem::Menu || NewTool == EWristToolkitItem::Count)
	{
		return;
	}

	if (bHasSelectedTool && SelectedTool == NewTool)
	{
		return;
	}

	SelectedTool = NewTool;
	bHasSelectedTool = true;
	UpdateButtonColors();
	OnSelectedToolChanged.Broadcast(SelectedTool);
}

void AWristToolkit::ClearSelectedTool()
{
	if (!bHasSelectedTool)
	{
		return;
	}

	bHasSelectedTool = false;
	UpdateButtonColors();
}

void AWristToolkit::SetMountedHand(EOculusXRHandType NewMountedHand)
{
	if (NewMountedHand != EOculusXRHandType::HandLeft && NewMountedHand != EOculusXRHandType::HandRight)
	{
		return;
	}

	MountedHand = NewMountedHand;
	LayoutItems();
}
