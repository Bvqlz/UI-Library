/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ProductEditTools.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HandPointerUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProductHistorySubsystem.h"
#include "ProductPlacer.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParamName = FName(TEXT("Color"));
	const float RayLength = 1000.0f;
	const float SearchIntervalSeconds = 0.5f;

	const FLinearColor ValidColor(0.3f, 1.0f, 0.4f);
	const FLinearColor InvalidColor(1.0f, 0.3f, 0.3f);
	const FLinearColor NeutralColor(1.0f, 1.0f, 1.0f);

	bool IsProductTool(EWristToolkitItem Tool)
	{
		return Tool == EWristToolkitItem::Move || Tool == EWristToolkitItem::Rotate || Tool == EWristToolkitItem::ScaleDimensions;
	}

	float AngleOnFloorDeg(const FVector& From, const FVector& To)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(To.Y - From.Y, To.X - From.X));
	}
}

AProductEditTools::AProductEditTools()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the placer, so this frame's pinch state is already known
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(RootComponent);
	DestinationMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DestinationMarker->SetCastShadow(false);
	// A flat disc, 50 cm across
	DestinationMarker->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.02f));
	DestinationMarker->SetVisibility(false);
	if (CylinderMesh.Succeeded())
	{
		DestinationMarker->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		DestinationMarker->SetMaterial(0, ShapeMaterial.Object);
	}

	HintText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HintText"));
	HintText->SetupAttachment(RootComponent);
	HintText->SetHorizontalAlignment(EHTA_Center);
	HintText->SetVerticalAlignment(EVRTA_TextCenter);
	HintText->SetWorldSize(7.0f);
	HintText->SetCastShadow(false);
	HintText->SetVisibility(false);
}

void AProductEditTools::BeginPlay()
{
	Super::BeginPlay();

	MarkerMaterial = DestinationMarker->CreateDynamicMaterialInstance(0);
	History = GetWorld()->GetSubsystem<UProductHistorySubsystem>();
	FindCollaborators();
}

void AProductEditTools::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void AProductEditTools::FindCollaborators()
{
	UWorld* World = GetWorld();
	if (!Toolkit)
	{
		const TActorIterator<AWristToolkit> It(World);
		if (It)
		{
			Toolkit = *It;
		}
	}
	if (!Placer)
	{
		const TActorIterator<AProductPlacer> It(World);
		if (It)
		{
			Placer = *It;
			// The placer must have read this frame's pinch before this actor uses it
			AddTickPrerequisiteActor(Placer);
		}
	}
}

void AProductEditTools::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Toolkit || !Placer || !History)
	{
		CollaboratorSearchTimer -= DeltaTime;
		if (CollaboratorSearchTimer <= 0.0f)
		{
			CollaboratorSearchTimer = SearchIntervalSeconds;
			if (!History)
			{
				History = GetWorld()->GetSubsystem<UProductHistorySubsystem>();
			}
			FindCollaborators();
		}
		return;
	}

	HandleEditorKeys();

	if (MessageTimeRemaining > 0.0f)
	{
		MessageTimeRemaining -= DeltaTime;
	}

	// Tools pause while the menu is open or a product is being placed, and a tool must be selected
	if (!Toolkit->HasSelectedTool() || Placer->IsPlacing() || Placer->IsUiOwningInput())
	{
		Deactivate();
		return;
	}

	const EWristToolkitItem Tool = Toolkit->GetSelectedTool();
	if (!bHasActiveTool || Tool != ActiveTool)
	{
		EnterTool(Tool);
	}

	if (IsProductTool(Tool))
	{
		TickProductTool(Tool);
	}
	else if (Tool == EWristToolkitItem::Navigation)
	{
		TickNavigation();
	}

	FaceHintToCamera();
}

void AProductEditTools::HandleEditorKeys()
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// ActivateItem is the same path as a poke, so pressing a key twice puts the tool away
	if (PlayerController->WasInputKeyJustPressed(EKeys::G))
	{
		Toolkit->ActivateItem(EWristToolkitItem::Move);
	}
	if (PlayerController->WasInputKeyJustPressed(EKeys::R))
	{
		Toolkit->ActivateItem(EWristToolkitItem::Rotate);
	}
	if (PlayerController->WasInputKeyJustPressed(EKeys::N))
	{
		Toolkit->ActivateItem(EWristToolkitItem::Navigation);
	}
	if (PlayerController->WasInputKeyJustPressed(EKeys::B))
	{
		Toolkit->ActivateItem(EWristToolkitItem::ScaleDimensions);
	}
}

void AProductEditTools::Deactivate()
{
	if (bDragging)
	{
		FinishDrag(false);
	}
	if (IsValid(Placer))
	{
		Placer->SetToolOwnsPinch(false);
	}
	bHasActiveTool = false;
	bNavPinchStarted = false;
	DestinationMarker->SetVisibility(false);
	HideHint();
}

void AProductEditTools::EnterTool(EWristToolkitItem Tool)
{
	if (bDragging)
	{
		FinishDrag(false);
	}
	bHasActiveTool = true;
	ActiveTool = Tool;
	bNavPinchStarted = false;
	MessageTimeRemaining = 0.0f;
	DestinationMarker->SetVisibility(false);
	HideHint();
	// Navigation uses every pinch; the product tools only claim the gesture once a drag starts
	Placer->SetToolOwnsPinch(Tool == EWristToolkitItem::Navigation);
}

// ---------------------------------------------------------------------------------------------
// Move, Rotate, Scale / Dimensions
// ---------------------------------------------------------------------------------------------

bool AProductEditTools::RayHitsProduct(const AFurnitureProduct* Product) const
{
	FVector Origin;
	FVector Direction;
	bool bFromHand = false;
	if (!Product || !HandPointerUtils::GetRay(GetWorld(), Placer->RayHand, Origin, Direction, bFromHand))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProductEditPointer), false, this);
	return GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * RayLength, ECC_Visibility, Params) &&
		Hit.GetActor() == Product;
}

bool AProductEditTools::CanUseTool(EWristToolkitItem Tool, const AFurnitureProduct* Product, FText& OutReason) const
{
	const FProductCapabilities& Capabilities = Product->Definition.Capabilities;
	switch (Tool)
	{
		case EWristToolkitItem::Move:
			if (!Capabilities.bMovable)
			{
				OutReason = FText::FromString(TEXT("This product can't be moved"));
				return false;
			}
			break;
		case EWristToolkitItem::Rotate:
			if (!Capabilities.bRotatable)
			{
				OutReason = FText::FromString(TEXT("This product can't be rotated"));
				return false;
			}
			break;
		case EWristToolkitItem::ScaleDimensions:
			if (!Capabilities.bResizable || Capabilities.MaxWidthCm <= Capabilities.MinWidthCm)
			{
				OutReason = FText::FromString(TEXT("This product can't be resized"));
				return false;
			}
			break;
		default:
			break;
	}
	return true;
}

void AProductEditTools::TickProductTool(EWristToolkitItem Tool)
{
	AFurnitureProduct* Selected = Placer->GetSelectedProduct();

	if (bDragging)
	{
		UpdateDrag();
		return;
	}

	// Not dragging: a selection change or a stale pinch must never be swallowed
	Placer->SetToolOwnsPinch(false);

	const TCHAR* Verb = Tool == EWristToolkitItem::Move ? TEXT("move")
		: Tool == EWristToolkitItem::Rotate ? TEXT("rotate")
		: TEXT("resize");

	if (!IsValid(Selected))
	{
		const FText Text = FText::FromString(FString::Printf(TEXT("Select a product to %s"), Verb));
		if (MessageTimeRemaining > 0.0f)
		{
			SetHint(MessageText, MessageColor, InFrontOfCamera(70.0f));
		}
		else
		{
			SetHint(Text, NeutralColor, InFrontOfCamera(70.0f));
		}
		return;
	}

	const FVector HintLocation = Selected->GetActorLocation() + FVector(0.0f, 0.0f, Selected->Definition.SizeCm.Z + 15.0f);
	if (MessageTimeRemaining > 0.0f)
	{
		SetHint(MessageText, MessageColor, HintLocation);
	}
	else
	{
		SetHint(FText::FromString(FString::Printf(TEXT("Pinch and drag to %s"), Verb)), NeutralColor, HintLocation);
	}

	// A hand starts with a fresh pinch; without a headset Space stands in for it
	const bool bHandStart = Placer->WasPinchPressed();
	const bool bKeyStart = Placer->WasConfirmKeyPressed();
	if (!(bHandStart || bKeyStart) || !RayHitsProduct(Selected))
	{
		return;
	}

	FText Reason;
	if (!CanUseTool(Tool, Selected, Reason))
	{
		ShowMessage(Reason, InvalidColor);
		return;
	}

	FVector FloorHit;
	if (!Placer->TryGetFloorHit(FloorHit))
	{
		ShowMessage(FText::FromString(TEXT("Aim down at the floor")), InvalidColor);
		return;
	}

	BeginDrag(Tool, Selected, FloorHit, !bHandStart);
}

void AProductEditTools::BeginDrag(EWristToolkitItem Tool, AFurnitureProduct* Product, const FVector& FloorHit, bool bKeyboard)
{
	bDragging = true;
	bKeyboardDrag = bKeyboard;
	bDragValid = true;
	DragTool = Tool;
	DragProduct = Product;
	DragStartTransform = Product->GetActorTransform();
	DragStartDefinition = Product->Definition;
	DragReason = FText::GetEmpty();

	const FVector Location = Product->GetActorLocation();
	switch (Tool)
	{
		case EWristToolkitItem::Move:
			DragGrabOffset = Location - FloorHit;
			break;
		case EWristToolkitItem::Rotate:
			DragStartAngleDeg = AngleOnFloorDeg(Location, FloorHit);
			DragStartYawDeg = Product->GetActorRotation().Yaw;
			break;
		case EWristToolkitItem::ScaleDimensions:
		{
			FVector Axis = Product->GetActorRightVector();
			Axis.Z = 0.0f;
			DragWidthAxis = Axis.GetSafeNormal();
			DragStartProjection = FVector::DotProduct(FloorHit - Location, DragWidthAxis);
			// Grabbing a side widens the product when pulled outward; near the middle, pulling toward +Y widens it
			DragWidthSign = DragStartProjection < 0.0f ? -1.0f : 1.0f;
			break;
		}
		default:
			break;
	}

	// The gesture now belongs to the tool, so releasing it never selects or clears anything
	Placer->SetToolOwnsPinch(true);
	MessageTimeRemaining = 0.0f;
	ApplyDragPose(FloorHit);
}

void AProductEditTools::UpdateDrag()
{
	AFurnitureProduct* Product = DragProduct.Get();
	if (!IsValid(Product) || Product != Placer->GetSelectedProduct())
	{
		FinishDrag(false);
		return;
	}

	bool bFinish = false;
	if (bKeyboardDrag)
	{
		if (Placer->WasCancelKeyPressed())
		{
			FinishDrag(false);
			return;
		}
		bFinish = Placer->WasConfirmKeyPressed();
	}
	else if (Placer->WasPinchReleased())
	{
		bFinish = true;
	}
	else if (!Placer->IsPinching())
	{
		// Tracking was lost mid-drag. That is never a deliberate release, so nothing is committed
		FinishDrag(false);
		return;
	}

	// Keep the last pose when the ray leaves the floor
	FVector FloorHit;
	if (Placer->TryGetFloorHit(FloorHit))
	{
		ApplyDragPose(FloorHit);
	}

	if (bFinish)
	{
		FinishDrag(true);
	}
}

void AProductEditTools::ApplyDragPose(const FVector& FloorHit)
{
	AFurnitureProduct* Product = DragProduct.Get();
	if (!IsValid(Product))
	{
		return;
	}

	switch (DragTool)
	{
		case EWristToolkitItem::Move:
		{
			FVector Location = Placer->SnapToGrid(FloorHit + DragGrabOffset);
			Location.Z = DragStartTransform.GetLocation().Z;
			Product->SetActorLocation(Location);
			DragLabel = TEXT("Release to place");
			break;
		}
		case EWristToolkitItem::Rotate:
		{
			const float Angle = AngleOnFloorDeg(Product->GetActorLocation(), FloorHit);
			// Snap the change, not the result, so grabbing a product never makes it jump
			float Delta = FRotator::NormalizeAxis(Angle - DragStartAngleDeg);
			if (RotationStepDeg > 0.0f)
			{
				Delta = FMath::GridSnap(Delta, RotationStepDeg);
			}
			const float Yaw = DragStartYawDeg + Delta;
			Product->SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
			DragLabel = FString::Printf(TEXT("Rotation %d deg"), FMath::RoundToInt(FRotator::NormalizeAxis(Yaw)));
			break;
		}
		case EWristToolkitItem::ScaleDimensions:
		{
			const FProductCapabilities& Capabilities = DragStartDefinition.Capabilities;
			const float Projection = FVector::DotProduct(FloorHit - Product->GetActorLocation(), DragWidthAxis);
			float Change = 2.0f * DragWidthSign * (Projection - DragStartProjection);
			if (Placer->GridStepCm > 0.0f)
			{
				Change = FMath::GridSnap(Change, Placer->GridStepCm);
			}
			const float Width = FMath::Clamp(DragStartDefinition.SizeCm.Y + Change, Capabilities.MinWidthCm, Capabilities.MaxWidthCm);

			// Rebuilding the parts is the costly step, so only do it when the snapped width changes
			if (!FMath::IsNearlyEqual(Product->Definition.SizeCm.Y, Width, 0.01f))
			{
				FProductDefinition Resized = Product->Definition;
				Resized.SizeCm.Y = Width;
				Product->InitializeFromDefinition(Resized);
			}
			DragLabel = FString::Printf(TEXT("Width %d cm"), FMath::RoundToInt(Width));
			break;
		}
		default:
			break;
	}

	FText Reason;
	bDragValid = Placer->ValidateFootprint(Product->GetActorLocation(), Product->GetFootprintHalfExtent(), Product, Reason);
	DragReason = Reason;
	Product->SetVisualState(bDragValid ? EProductVisualState::GhostValid : EProductVisualState::GhostInvalid);

	const FVector HintLocation = Product->GetActorLocation() + FVector(0.0f, 0.0f, Product->Definition.SizeCm.Z + 15.0f);
	if (bDragValid)
	{
		SetHint(FText::FromString(DragLabel), ValidColor, HintLocation);
	}
	else
	{
		SetHint(DragReason, InvalidColor, HintLocation);
	}
}

void AProductEditTools::RestoreStartState(AFurnitureProduct* Product)
{
	if (!IsValid(Product))
	{
		return;
	}

	Product->SetActorTransform(DragStartTransform);
	if (!Product->Definition.SizeCm.Equals(DragStartDefinition.SizeCm, 0.01f))
	{
		Product->InitializeFromDefinition(DragStartDefinition);
	}
}

void AProductEditTools::FinishDrag(bool bCommit)
{
	if (!bDragging)
	{
		return;
	}
	bDragging = false;

	AFurnitureProduct* Product = DragProduct.Get();
	DragProduct = nullptr;
	if (IsValid(Placer))
	{
		Placer->SetToolOwnsPinch(false);
	}
	if (!IsValid(Product))
	{
		return;
	}

	const FTransform AfterTransform = Product->GetActorTransform();
	const FProductDefinition AfterDefinition = Product->Definition;
	const bool bShouldCommit = bCommit && bDragValid && History;
	const FText Reason = DragReason;

	// Always go back to the starting state first. A commit then re-applies the new state through the
	// history, so the history sees a real before and after
	RestoreStartState(Product);
	Product->SetVisualState(EProductVisualState::Selected);

	HideHint();
	if (bShouldCommit)
	{
		History->ModifyProduct(Product, AfterTransform, AfterDefinition);
	}
	else if (bCommit && !bDragValid)
	{
		ShowMessage(Reason, InvalidColor);
	}
}

// ---------------------------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------------------------

bool AProductEditTools::ValidateTeleport(const FVector& Target, FText& OutReason) const
{
	FVector Camera;
	if (!HandPointerUtils::GetCameraLocation(GetWorld(), Camera))
	{
		OutReason = FText::FromString(TEXT("No player to move"));
		return false;
	}

	if (FVector::Dist2D(Camera, Target) > MaxTeleportDistanceCm)
	{
		OutReason = FText::FromString(TEXT("Too far away"));
		return false;
	}

	for (const AFurnitureProduct* Product : History->GetProducts())
	{
		const FVector2D Half = Product->GetFootprintHalfExtent();
		if (FMath::Abs(Target.X - Product->GetActorLocation().X) < Half.X + PlayerClearanceCm &&
			FMath::Abs(Target.Y - Product->GetActorLocation().Y) < Half.Y + PlayerClearanceCm)
		{
			OutReason = FText::FromString(FString::Printf(TEXT("Blocked by the %s"), *Product->Definition.DisplayName.ToString()));
			return false;
		}
	}

	return true;
}

void AProductEditTools::Teleport(const FVector& Target)
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	FVector Camera;
	if (!Pawn || !HandPointerUtils::GetCameraLocation(GetWorld(), Camera))
	{
		ShowMessage(FText::FromString(TEXT("No player to move")), InvalidColor);
		return;
	}

	// Put the player's head over the destination. The pawn is the tracking origin, so move it by the
	// difference rather than to the point, which keeps the room-scale offset of the headset
	FVector Delta = Target - Camera;
	Delta.Z = 0.0f;
	Pawn->SetActorLocation(Pawn->GetActorLocation() + Delta, false, nullptr, ETeleportType::TeleportPhysics);
}

void AProductEditTools::TickNavigation()
{
	// Navigation owns every pinch while selected, so the placer never selects behind it
	Placer->SetToolOwnsPinch(true);

	if (Placer->WasPinchPressed())
	{
		bNavPinchStarted = true;
	}
	// A pinch that began before the tool was picked cannot teleport; only a fresh press and release can
	const bool bConfirm = (Placer->WasPinchReleased() && bNavPinchStarted) || Placer->WasConfirmKeyPressed();
	if (!Placer->IsPinching())
	{
		bNavPinchStarted = false;
	}

	FVector Hit;
	if (!Placer->TryGetFloorHit(Hit))
	{
		DestinationMarker->SetVisibility(false);
		if (MessageTimeRemaining > 0.0f)
		{
			SetHint(MessageText, MessageColor, InFrontOfCamera(70.0f));
		}
		else
		{
			SetHint(FText::FromString(TEXT("Aim down at the floor")), NeutralColor, InFrontOfCamera(70.0f));
		}
		return;
	}

	FText Reason;
	const bool bValid = ValidateTeleport(Hit, Reason);
	const FLinearColor Color = bValid ? ValidColor : InvalidColor;

	DestinationMarker->SetWorldLocation(FVector(Hit.X, Hit.Y, Placer->PlacementAreaCenter.Z + 0.5f));
	DestinationMarker->SetVisibility(true);
	if (MarkerMaterial)
	{
		MarkerMaterial->SetVectorParameterValue(ColorParamName, Color);
	}

	const FVector HintLocation = Hit + FVector(0.0f, 0.0f, 30.0f);
	if (MessageTimeRemaining > 0.0f)
	{
		SetHint(MessageText, MessageColor, HintLocation);
	}
	else if (bValid)
	{
		SetHint(FText::FromString(TEXT("Pinch and release to go here")), Color, HintLocation);
	}
	else
	{
		SetHint(Reason, Color, HintLocation);
	}

	if (bConfirm)
	{
		if (bValid)
		{
			Teleport(Hit);
		}
		else
		{
			ShowMessage(Reason, InvalidColor);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Hint text
// ---------------------------------------------------------------------------------------------

void AProductEditTools::ShowMessage(const FText& Text, const FLinearColor& Color, float DurationSeconds)
{
	MessageText = Text;
	MessageColor = Color;
	MessageTimeRemaining = DurationSeconds;
}

void AProductEditTools::SetHint(const FText& Text, const FLinearColor& Color, const FVector& WorldLocation)
{
	HintText->SetText(Text);
	HintText->SetTextRenderColor(Color.ToFColor(true));
	HintText->SetWorldLocation(WorldLocation);
	HintText->SetVisibility(true);
}

void AProductEditTools::HideHint()
{
	HintText->SetVisibility(false);
}

FVector AProductEditTools::InFrontOfCamera(float DistanceCm) const
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		return PlayerController->PlayerCameraManager->GetCameraLocation() +
			PlayerController->PlayerCameraManager->GetCameraRotation().Vector() * DistanceCm;
	}
	return GetActorLocation();
}

void AProductEditTools::FaceHintToCamera()
{
	FVector CameraLocation;
	if (HintText->IsVisible() && HandPointerUtils::GetCameraLocation(GetWorld(), CameraLocation))
	{
		HintText->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(HintText->GetComponentLocation(), CameraLocation));
	}
}
