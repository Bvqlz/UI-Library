/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ProductPlacer.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "HandPointerUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProductHistorySubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParamName = FName(TEXT("Color"));
	const float RayLength = 1000.0f;
}

AProductPlacer::AProductPlacer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	AreaMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AreaMarker"));
	AreaMarker->SetupAttachment(RootComponent);
	AreaMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaMarker->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		AreaMarker->SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		AreaMarker->SetMaterial(0, ShapeMaterial.Object);
	}

	HintText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HintText"));
	HintText->SetupAttachment(RootComponent);
	HintText->SetHorizontalAlignment(EHTA_Center);
	HintText->SetVerticalAlignment(EVRTA_TextCenter);
	HintText->SetWorldSize(7.0f);
	HintText->SetCastShadow(false);
	HintText->SetVisibility(false);
}

void AProductPlacer::BeginPlay()
{
	Super::BeginPlay();

	// The marked placement area: a thin slab on the floor
	AreaMarker->SetWorldLocation(PlacementAreaCenter + FVector(0.0f, 0.0f, 0.3f));
	AreaMarker->SetWorldScale3D(FVector(PlacementAreaHalfExtent.X * 2.0f, PlacementAreaHalfExtent.Y * 2.0f, 0.6f) * 0.01f);
	if (UMaterialInstanceDynamic* Material = AreaMarker->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(ColorParamName, FLinearColor(0.1f, 0.45f, 0.2f));
	}
}

void AProductPlacer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	bKeyConfirm = PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::SpaceBar);
	bKeyCancel = PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::Escape);

	UpdatePinch();
	if (bPlacing)
	{
		UpdatePlacing();
	}
	else
	{
		UpdateIdle();
	}
	FaceHintToCamera();
}

void AProductPlacer::UpdatePinch()
{
	const bool bWasPinching = bPinching;

	float Distance = 0.0f;
	if (HandPointerUtils::GetPinchDistance(GetWorld(), RayHand, Distance))
	{
		// Different thresholds to start and end a pinch, so noise cannot flicker it
		if (!bPinching && Distance < PinchStartDistance)
		{
			bPinching = true;
		}
		else if (bPinching && Distance > PinchReleaseDistance)
		{
			bPinching = false;
		}
		bPinchPressed = bPinching && !bWasPinching;
		bPinchReleased = !bPinching && bWasPinching;
	}
	else
	{
		// Lost tracking is never a deliberate release: abandon the gesture without committing
		bPinching = false;
		bPinchPressed = false;
		bPinchReleased = false;
		bPinchStartedInMode = false;
	}

	if (bAwaitingFreshPinch && !bPinching)
	{
		bAwaitingFreshPinch = false;
	}
	if (bPinchPressed && !bAwaitingFreshPinch)
	{
		bPinchStartedInMode = true;
	}
}

bool AProductPlacer::TryGetFloorHit(FVector& OutHit) const
{
	FVector Origin;
	FVector Direction;
	bool bFromHand = false;
	if (!HandPointerUtils::GetRay(GetWorld(), RayHand, Origin, Direction, bFromHand) || Direction.Z > -0.01f)
	{
		return false;
	}

	const float T = (PlacementAreaCenter.Z - Origin.Z) / Direction.Z;
	if (T < 0.0f || T > RayLength * 3.0f)
	{
		return false;
	}

	OutHit = Origin + Direction * T;
	return true;
}

bool AProductPlacer::ValidatePlacement(const FVector& Location, FText& OutReason) const
{
	if (!Ghost)
	{
		return false;
	}

	const FVector2D Half = Ghost->GetFootprintHalfExtent();
	if (FMath::Abs(Location.X - PlacementAreaCenter.X) + Half.X > PlacementAreaHalfExtent.X ||
		FMath::Abs(Location.Y - PlacementAreaCenter.Y) + Half.Y > PlacementAreaHalfExtent.Y)
	{
		OutReason = FText::FromString(TEXT("Outside the placement area"));
		return false;
	}

	if (const UProductHistorySubsystem* History = GetWorld()->GetSubsystem<UProductHistorySubsystem>())
	{
		for (const AFurnitureProduct* Other : History->GetProducts())
		{
			const FVector2D OtherHalf = Other->GetFootprintHalfExtent();
			if (FMath::Abs(Location.X - Other->GetActorLocation().X) < Half.X + OtherHalf.X &&
				FMath::Abs(Location.Y - Other->GetActorLocation().Y) < Half.Y + OtherHalf.Y)
			{
				OutReason = FText::FromString(FString::Printf(TEXT("Overlaps the %s"), *Other->Definition.DisplayName.ToString()));
				return false;
			}
		}
	}

	return true;
}

void AProductPlacer::UpdatePlacing()
{
	if (bKeyCancel)
	{
		CancelPlacement();
		return;
	}

	FVector Hit;
	if (!TryGetFloorHit(Hit))
	{
		Ghost->SetActorHiddenInGame(true);
		bGhostValid = false;
		HintText->SetVisibility(false);
		return;
	}

	FVector Location = Hit;
	if (GridStepCm > 0.0f)
	{
		Location.X = FMath::GridSnap(Location.X, GridStepCm);
		Location.Y = FMath::GridSnap(Location.Y, GridStepCm);
	}
	Location.Z = PlacementAreaCenter.Z;

	Ghost->SetActorHiddenInGame(false);
	Ghost->SetActorLocation(Location);

	FText Reason;
	bGhostValid = ValidatePlacement(Location, Reason);
	Ghost->SetVisualState(bGhostValid ? EProductVisualState::GhostValid : EProductVisualState::GhostInvalid);

	const FVector HintLocation = Location + FVector(0.0f, 0.0f, PlacingDefinition.SizeCm.Z + 15.0f);
	if (bGhostValid)
	{
		SetHint(FText::FromString(TEXT("Pinch and release to place")), FLinearColor(0.3f, 1.0f, 0.4f), HintLocation);
	}
	else
	{
		SetHint(Reason, FLinearColor(1.0f, 0.3f, 0.3f), HintLocation);
	}

	// Only a pinch that began after placement started can confirm it
	const bool bConfirmed = (bPinchReleased && bPinchStartedInMode) || bKeyConfirm;
	if (bPinchReleased)
	{
		bPinchStartedInMode = false;
	}
	if (bConfirmed)
	{
		ConfirmPlacement();
	}

#if ENABLE_DRAW_DEBUG
	FVector Origin;
	FVector Direction;
	bool bFromHand = false;
	if (HandPointerUtils::GetRay(GetWorld(), RayHand, Origin, Direction, bFromHand) && bFromHand)
	{
		DrawDebugLine(GetWorld(), Origin, Hit, bGhostValid ? FColor::Green : FColor::Red, false, -1.0f, 0, 0.3f);
	}
#endif
}

void AProductPlacer::ConfirmPlacement()
{
	if (!Ghost || !bGhostValid)
	{
		// Invalid placement cannot be committed; the preview stays so the user can adjust
		return;
	}

	UProductHistorySubsystem* History = GetWorld()->GetSubsystem<UProductHistorySubsystem>();
	if (!History)
	{
		return;
	}

	const FTransform Transform = Ghost->GetActorTransform();
	AFurnitureProduct* Placed = History->PlaceProduct(PlacingDefinition, Transform);

	CancelPlacement();
	if (Placed)
	{
		SelectProduct(Placed);
	}
}

void AProductPlacer::UpdateIdle()
{
	if (bUiOwnsInput)
	{
		SetHovered(nullptr);
		return;
	}

	FVector Origin;
	FVector Direction;
	bool bFromHand = false;
	AFurnitureProduct* Candidate = nullptr;
	if (HandPointerUtils::GetRay(GetWorld(), RayHand, Origin, Direction, bFromHand))
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ProductPointer), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * RayLength, ECC_Visibility, Params))
		{
			Candidate = Cast<AFurnitureProduct>(Hit.GetActor());
		}

#if ENABLE_DRAW_DEBUG
		if (bFromHand)
		{
			DrawDebugLine(GetWorld(), Origin, Origin + Direction * (Candidate ? FVector::Distance(Origin, Hit.ImpactPoint) : 150.0f),
				Candidate ? FColor::Yellow : FColor::White, false, -1.0f, 0, 0.2f);
		}
#endif
	}
	SetHovered(Candidate);

	// Point to preview, pinch and release to select; releasing over empty space clears the selection
	const bool bSelectGesture = (bPinchReleased && bPinchStartedInMode) || bKeyConfirm;
	if (bPinchReleased)
	{
		bPinchStartedInMode = false;
	}
	if (bSelectGesture)
	{
		if (Candidate)
		{
			SelectProduct(Candidate);
		}
		else
		{
			ClearSelection();
		}
	}
}

void AProductPlacer::SetHovered(AFurnitureProduct* Product)
{
	AFurnitureProduct* Previous = HoveredProduct.Get();
	if (Previous == Product)
	{
		return;
	}

	if (IsValid(Previous) && Previous != SelectedProduct.Get())
	{
		Previous->SetVisualState(EProductVisualState::Normal);
	}
	HoveredProduct = Product;
	if (IsValid(Product) && Product != SelectedProduct.Get())
	{
		Product->SetVisualState(EProductVisualState::Hovered);
	}
}

void AProductPlacer::SelectProduct(AFurnitureProduct* Product)
{
	AFurnitureProduct* Previous = SelectedProduct.Get();
	if (Previous == Product)
	{
		return;
	}

	if (IsValid(Previous))
	{
		Previous->SetVisualState(EProductVisualState::Normal);
	}
	SelectedProduct = Product;
	if (IsValid(Product))
	{
		Product->SetVisualState(EProductVisualState::Selected);
	}
	OnSelectionChanged.Broadcast(Product);
}

void AProductPlacer::ClearSelection()
{
	SelectProduct(nullptr);
}

void AProductPlacer::SetUiOwnsInput(bool bInUiOwnsInput)
{
	bUiOwnsInput = bInUiOwnsInput;
	if (bUiOwnsInput)
	{
		// A gesture that started on the menu must not select a room object when it ends
		bPinchStartedInMode = false;
	}
}

void AProductPlacer::BeginPlacement(const FProductDefinition& Definition)
{
	CancelPlacement();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ghost = GetWorld()->SpawnActor<AFurnitureProduct>(
		AFurnitureProduct::StaticClass(), PlacementAreaCenter, FRotator(0.0f, PlacementYaw, 0.0f), SpawnParameters);
	if (!Ghost)
	{
		return;
	}
	Ghost->InitializeFromDefinition(Definition);
	Ghost->SetGhost(true);

	PlacingDefinition = Definition;
	bPlacing = true;
	bGhostValid = false;
	// The pinch that may have been used to reach this point cannot also confirm the placement
	bAwaitingFreshPinch = bPinching;
	bPinchStartedInMode = false;
	HintText->SetVisibility(true);
	OnPlacingChanged.Broadcast(true);
}

void AProductPlacer::CancelPlacement()
{
	if (IsValid(Ghost))
	{
		Ghost->Destroy();
	}
	Ghost = nullptr;
	HintText->SetVisibility(false);
	bPinchStartedInMode = false;

	if (bPlacing)
	{
		bPlacing = false;
		OnPlacingChanged.Broadcast(false);
	}
}

void AProductPlacer::SetHint(const FText& Text, const FLinearColor& Color, const FVector& WorldLocation)
{
	HintText->SetText(Text);
	HintText->SetTextRenderColor(Color.ToFColor(true));
	HintText->SetWorldLocation(WorldLocation);
	HintText->SetVisibility(true);
}

void AProductPlacer::FaceHintToCamera()
{
	FVector CameraLocation;
	if (HintText->IsVisible() && HandPointerUtils::GetCameraLocation(GetWorld(), CameraLocation))
	{
		HintText->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(HintText->GetComponentLocation(), CameraLocation));
	}
}
