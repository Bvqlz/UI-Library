/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "WristMenu.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
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
	const float PanelThickness = 1.5f;
	const float PanelMargin = 4.5f;
	const float FingertipRadius = 0.6f;
	const float MessageSeconds = 3.0f;
	const int32 FirstContentSlot = 6;
}

AWristMenu::AWristMenu()
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

	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		UStaticMeshComponent* ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Button_%d"), SlotIndex));
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
			*FString::Printf(TEXT("Label_%d"), SlotIndex));
		Label->SetupAttachment(RootComponent);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextTop);
		Label->SetTextRenderColor(FColor::White);
		Label->SetCastShadow(false);
		ButtonLabels.Add(Label);
	}

	TitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Title"));
	TitleText->SetupAttachment(RootComponent);
	TitleText->SetHorizontalAlignment(EHTA_Center);
	TitleText->SetVerticalAlignment(EVRTA_TextBottom);
	TitleText->SetTextRenderColor(FColor(180, 220, 255));
	TitleText->SetCastShadow(false);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Status"));
	StatusText->SetupAttachment(RootComponent);
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetVerticalAlignment(EVRTA_TextTop);
	StatusText->SetTextRenderColor(FColor(255, 230, 140));
	StatusText->SetCastShadow(false);

	Slots.SetNum(NumSlots);
	LayoutPanel();
}

void AWristMenu::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutPanel();
}

FVector AWristMenu::GetSlotLocalLocation(int32 SlotIndex) const
{
	const int32 Column = SlotIndex % Columns;
	const int32 Row = SlotIndex / Columns;

	// Same convention as the button panel: the face points along +X, columns run along -Y when seen
	// from the front and rows run top to bottom along -Z
	const float Y = -(Column - (Columns - 1) * 0.5f) * ButtonSpacing;
	const float Z = -(Row - (Rows - 1) * 0.5f) * ButtonSpacing;
	return FVector(0.0f, Y, Z);
}

void AWristMenu::LayoutPanel()
{
	const float Width = (Columns - 1) * ButtonSpacing + 2.0f * (ButtonRadius + PanelMargin);
	// Extra room above for the title and below for the status line
	const float TextBand = 6.0f;
	const float Height = (Rows - 1) * ButtonSpacing + 2.0f * (ButtonRadius + PanelMargin) + 2.0f * TextBand;

	Backboard->SetRelativeLocation(FVector(-PanelThickness * 0.5f, 0.0f, 0.0f));
	Backboard->SetRelativeScale3D(FVector(PanelThickness, Width, Height) * 0.01f);

	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		const FVector Center = GetSlotLocalLocation(SlotIndex);
		ButtonMeshes[SlotIndex]->SetRelativeLocation(Center + FVector(ButtonHeight * 0.5f, 0.0f, 0.0f));
		ButtonMeshes[SlotIndex]->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		ButtonMeshes[SlotIndex]->SetRelativeScale3D(
			FVector(ButtonRadius * 2.0f, ButtonRadius * 2.0f, ButtonHeight) * 0.01f);

		ButtonLabels[SlotIndex]->SetWorldSize(LabelSize);
		ButtonLabels[SlotIndex]->SetRelativeLocation(Center + FVector(ButtonHeight + 0.1f, 0.0f, -(ButtonRadius + 0.5f)));
	}

	const float TopZ = (Rows - 1) * ButtonSpacing * 0.5f + ButtonRadius + 1.0f;
	TitleText->SetWorldSize(LabelSize * 1.5f);
	TitleText->SetRelativeLocation(FVector(ButtonHeight, 0.0f, TopZ));
	StatusText->SetWorldSize(LabelSize);
	StatusText->SetRelativeLocation(FVector(ButtonHeight, 0.0f, -TopZ - LabelSize - 1.0f));
}

const TArray<FProductDefinition>& AWristMenu::GetProducts() const
{
	return (Catalog && Catalog->Products.Num() > 0) ? Catalog->Products : DefaultProducts;
}

void AWristMenu::BeginPlay()
{
	Super::BeginPlay();

	DefaultProducts = UProductCatalogAsset::MakeDefaultProducts();

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
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		TimeSincePressed[SlotIndex] = PressCooldown;
	}

	History = GetWorld()->GetSubsystem<UProductHistorySubsystem>();
	if (History)
	{
		History->OnHistoryChanged.AddDynamic(this, &AWristMenu::HandleHistoryChanged);
	}

	LayoutPanel();
	RebuildSlots();
	RefreshVisuals();
	FindCollaborators();

	// The menu starts closed
	bOpen = false;
	SetActorHiddenInGame(true);
}

void AWristMenu::FindCollaborators()
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
	if (Toolkit && !bBoundToolkit)
	{
		Toolkit->OnItemActivated.AddDynamic(this, &AWristMenu::HandleToolkitItem);
		bBoundToolkit = true;
	}
	if (!Placer)
	{
		const TActorIterator<AProductPlacer> It(World);
		if (It)
		{
			Placer = *It;
			Placer->OnSelectionChanged.AddDynamic(this, &AWristMenu::HandleSelectionChanged);
			Placer->OnPlacingChanged.AddDynamic(this, &AWristMenu::HandlePlacingChanged);
		}
	}
}

void AWristMenu::HandleToolkitItem(EWristToolkitItem Item)
{
	// The toolkit's center item opens (or closes) the menu; the other four are not ours
	if (Item == EWristToolkitItem::Menu)
	{
		ToggleMenu();
	}
}

void AWristMenu::HandleSelectionChanged(AFurnitureProduct* /*SelectedProduct*/)
{
	bConfirmingRemove = false;
	RebuildSlots();
	RefreshVisuals();
}

void AWristMenu::HandleHistoryChanged()
{
	RebuildSlots();
	RefreshVisuals();
}

void AWristMenu::HandlePlacingChanged(bool /*bIsPlacing*/)
{
	RebuildSlots();
	RefreshVisuals();
}

void AWristMenu::OpenMenu()
{
	if (bOpen)
	{
		return;
	}

	bOpen = true;
	bConfirmingRemove = false;
	SetActorHiddenInGame(false);
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		bTipWasInFront[SlotIndex] = false;
		// Opening with a poke must not also press whatever sits under the finger
		TimeSincePressed[SlotIndex] = 0.0f;
	}
	if (Placer)
	{
		Placer->SetUiOwnsInput(true);
	}
	RebuildSlots();
	RefreshVisuals();
	OnOpenChanged.Broadcast(true);
}

void AWristMenu::CloseMenu()
{
	if (!bOpen)
	{
		return;
	}

	bOpen = false;
	bConfirmingRemove = false;
	SetActorHiddenInGame(true);
	if (Placer)
	{
		Placer->SetUiOwnsInput(false);
	}
	OnOpenChanged.Broadcast(false);
}

void AWristMenu::ToggleMenu()
{
	if (bOpen)
	{
		CloseMenu();
	}
	else
	{
		OpenMenu();
	}
}

void AWristMenu::SetPage(EWristMenuPage NewPage)
{
	Page = NewPage;
	bConfirmingRemove = false;
	RebuildSlots();
	RefreshVisuals();
}

void AWristMenu::RebuildSlots()
{
	Slots.Reset();
	Slots.SetNum(NumSlots);

	const bool bHasHistory = History != nullptr;
	const AFurnitureProduct* Selected = Placer ? Placer->GetSelectedProduct() : nullptr;

	// Row 1: Undo near the top, as the design doc asks
	Slots[0] = {FText::FromString(TEXT("Undo")), EMenuAction::Undo, 0, true,
		bHasHistory && History->CanUndo(), false, FText::FromString(TEXT("Nothing to undo"))};
	Slots[1] = {FText::FromString(TEXT("Redo")), EMenuAction::Redo, 0, true,
		bHasHistory && History->CanRedo(), false, FText::FromString(TEXT("Nothing to redo"))};
	Slots[2] = {FText::FromString(TEXT("Close")), EMenuAction::Close, 0, true, true, false, FText()};

	// Row 2: pages
	Slots[3] = {FText::FromString(TEXT("Products")), EMenuAction::PageProducts, 0, true, true, Page == EWristMenuPage::Products, FText()};
	Slots[4] = {FText::FromString(TEXT("Selected")), EMenuAction::PageSelected, 0, true, true, Page == EWristMenuPage::Selected, FText()};
	Slots[5] = {FText::FromString(TEXT("Settings")), EMenuAction::PageSettings, 0, true, true, Page == EWristMenuPage::Settings, FText()};

	switch (Page)
	{
		case EWristMenuPage::Products:
		{
			const TArray<FProductDefinition>& Products = GetProducts();
			for (int32 Index = 0; Index < Products.Num() && FirstContentSlot + Index < NumSlots; ++Index)
			{
				Slots[FirstContentSlot + Index] = {Products[Index].DisplayName, EMenuAction::ChooseProduct, Index, true, true, false, FText()};
			}
			break;
		}
		case EWristMenuPage::Selected:
		{
			const bool bCanRemove = Selected && Selected->Definition.Capabilities.bRemovable;
			FText Reason = FText::FromString(TEXT("Select a product first"));
			if (Selected && !bCanRemove)
			{
				Reason = FText::FromString(TEXT("This product cannot be removed"));
			}
			Slots[FirstContentSlot] = {FText::FromString(TEXT("Remove")), EMenuAction::Remove, 0, true,
				bCanRemove && !bConfirmingRemove, false, Reason};
			if (bConfirmingRemove)
			{
				Slots[FirstContentSlot + 1] = {FText::FromString(TEXT("Confirm")), EMenuAction::ConfirmRemove, 0, true, true, false, FText()};
				Slots[FirstContentSlot + 2] = {FText::FromString(TEXT("Cancel")), EMenuAction::CancelRemove, 0, true, true, false, FText()};
			}
			break;
		}
		case EWristMenuPage::Settings:
		{
			const bool bLeft = !Toolkit || Toolkit->MountedHand == EOculusXRHandType::HandLeft;
			Slots[FirstContentSlot] = {FText::FromString(bLeft ? TEXT("Hand: Left") : TEXT("Hand: Right")),
				EMenuAction::SwapHand, 0, true, Toolkit != nullptr, false, FText::FromString(TEXT("No wrist toolkit in the level"))};
			const float Step = StepChoicesCm.IsValidIndex(StepIndex) ? StepChoicesCm[StepIndex] : 5.0f;
			Slots[FirstContentSlot + 1] = {FText::FromString(FString::Printf(TEXT("Step: %g cm"), Step)),
				EMenuAction::CycleStep, 0, true, Placer != nullptr, false, FText::FromString(TEXT("No placement controller in the level"))};
			break;
		}
	}
}

void AWristMenu::RefreshVisuals()
{
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		const FMenuSlot& Slot = Slots[SlotIndex];
		ButtonMeshes[SlotIndex]->SetVisibility(Slot.bVisible);
		ButtonLabels[SlotIndex]->SetVisibility(Slot.bVisible);
		ButtonLabels[SlotIndex]->SetText(Slot.Label);

		if (SlotIndex < ButtonMaterials.Num() && ButtonMaterials[SlotIndex])
		{
			FLinearColor Color = ButtonColor;
			if (!Slot.bEnabled)
			{
				Color = DisabledColor;
			}
			else if (SlotIndex == HoveredSlot)
			{
				Color = HoverColor;
			}
			else if (Slot.bActive)
			{
				Color = ActiveColor;
			}
			ButtonMaterials[SlotIndex]->SetVectorParameterValue(ColorParamName, Color);
		}
	}

	const TCHAR* PageName = TEXT("Products");
	FString Hint = TEXT("Poke a product to preview it");
	const AFurnitureProduct* Selected = Placer ? Placer->GetSelectedProduct() : nullptr;
	if (Page == EWristMenuPage::Selected)
	{
		PageName = TEXT("Selected");
		if (bConfirmingRemove && Selected)
		{
			Hint = FString::Printf(TEXT("Remove the %s?"), *Selected->Definition.DisplayName.ToString());
		}
		else if (Selected)
		{
			Hint = FString::Printf(TEXT("%s selected"), *Selected->Definition.DisplayName.ToString());
		}
		else
		{
			Hint = TEXT("Nothing selected: point, pinch, release");
		}
	}
	else if (Page == EWristMenuPage::Settings)
	{
		PageName = TEXT("Settings");
		Hint = TEXT("Poke to change a setting");
	}

	TitleText->SetText(FText::FromString(FString::Printf(TEXT("MENU - %s"), PageName)));
	StatusText->SetText(MessageTimeLeft > 0.0f ? Message : FText::FromString(Hint));
}

void AWristMenu::ShowMessage(const FText& NewMessage)
{
	Message = NewMessage;
	MessageTimeLeft = MessageSeconds;
	RefreshVisuals();
}

void AWristMenu::PressSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= NumSlots || !Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex].bVisible)
	{
		return;
	}

	TimeSincePressed[SlotIndex] = 0.0f;
	if (!Slots[SlotIndex].bEnabled)
	{
		// Unavailable controls stay recognizable and say why
		ShowMessage(Slots[SlotIndex].DisabledReason);
		return;
	}
	ActivateSlot(SlotIndex);
}

void AWristMenu::ActivateSlot(int32 SlotIndex)
{
	const FMenuSlot Slot = Slots[SlotIndex];
	switch (Slot.Action)
	{
		case EMenuAction::Undo:
			if (History)
			{
				History->Undo();
			}
			break;
		case EMenuAction::Redo:
			if (History)
			{
				History->Redo();
			}
			break;
		case EMenuAction::Close:
			CloseMenu();
			return;
		case EMenuAction::PageProducts:
			SetPage(EWristMenuPage::Products);
			return;
		case EMenuAction::PageSelected:
			SetPage(EWristMenuPage::Selected);
			return;
		case EMenuAction::PageSettings:
			SetPage(EWristMenuPage::Settings);
			return;
		case EMenuAction::ChooseProduct:
		{
			const TArray<FProductDefinition>& Products = GetProducts();
			if (Products.IsValidIndex(Slot.Param) && Placer)
			{
				const FProductDefinition Product = Products[Slot.Param];
				OnProductChosen.Broadcast(Product);
				// A new pinch and release in the room confirms; picking from the catalog never does
				Placer->BeginPlacement(Product);
				CloseMenu();
			}
			return;
		}
		case EMenuAction::Remove:
			bConfirmingRemove = true;
			break;
		case EMenuAction::ConfirmRemove:
		{
			bConfirmingRemove = false;
			AFurnitureProduct* Selected = Placer ? Placer->GetSelectedProduct() : nullptr;
			if (Selected && History)
			{
				History->RemoveProduct(Selected);
				Placer->ClearSelection();
				ShowMessage(FText::FromString(TEXT("Removed. Undo restores it.")));
			}
			break;
		}
		case EMenuAction::CancelRemove:
			bConfirmingRemove = false;
			break;
		case EMenuAction::SwapHand:
			if (Toolkit)
			{
				const bool bWasLeft = Toolkit->MountedHand == EOculusXRHandType::HandLeft;
				Toolkit->SetMountedHand(bWasLeft ? EOculusXRHandType::HandRight : EOculusXRHandType::HandLeft);
				if (Placer)
				{
					// The pointing hand is always the one that does not wear the toolkit
					Placer->RayHand = bWasLeft ? EOculusXRHandType::HandLeft : EOculusXRHandType::HandRight;
				}
			}
			break;
		case EMenuAction::CycleStep:
			if (Placer && StepChoicesCm.Num() > 0)
			{
				StepIndex = (StepIndex + 1) % StepChoicesCm.Num();
				Placer->GridStepCm = StepChoicesCm[StepIndex];
			}
			break;
		default:
			break;
	}

	RebuildSlots();
	RefreshVisuals();
}

EOculusXRHandType AWristMenu::GetPokeHand() const
{
	if (Toolkit)
	{
		return Toolkit->MountedHand == EOculusXRHandType::HandLeft ? EOculusXRHandType::HandRight : EOculusXRHandType::HandLeft;
	}
	return EOculusXRHandType::HandRight;
}

void AWristMenu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Toolkit || !Placer)
	{
		FindCollaborators();
	}
	HandleDebugKeys();

	if (MessageTimeLeft > 0.0f)
	{
		MessageTimeLeft -= DeltaTime;
		if (MessageTimeLeft <= 0.0f)
		{
			RefreshVisuals();
		}
	}

	if (!bOpen)
	{
		return;
	}

	UpdateAnchor();
	UpdatePoke(DeltaTime);
	UpdateLabelFacing();
}

void AWristMenu::UpdateAnchor()
{
	FVector CameraLocation;
	if (!HandPointerUtils::GetCameraLocation(GetWorld(), CameraLocation))
	{
		return;
	}

	FVector Anchor;
	if (Toolkit && !Toolkit->IsHidden())
	{
		// Hand-anchored: follow the wrist toolkit
		Anchor = Toolkit->GetActorLocation() + Toolkit->GetActorUpVector() * AnchorHeight;
	}
	else
	{
		// No tracked wrist (for example in the editor): float in front of the camera instead
		const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		const FVector Forward = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
		Anchor = CameraLocation + Forward * FallbackDistance - FVector(0.0f, 0.0f, 8.0f);
	}

	// The face (+X) always turns toward the viewer
	SetActorLocationAndRotation(Anchor, UKismetMathLibrary::FindLookAtRotation(Anchor, CameraLocation));
}

void AWristMenu::UpdatePoke(float DeltaTime)
{
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		TimeSincePressed[SlotIndex] += DeltaTime;
	}

	FVector TipWorld;
	const bool bHasTip = HandPointerUtils::GetBoneLocation(GetWorld(), GetPokeHand(), EOculusXRBone::Index_Tip, TipWorld);
	const FVector Tip = GetActorTransform().InverseTransformPosition(TipWorld);

	int32 NewHovered = -1;
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		if (!Slots[SlotIndex].bVisible || !bHasTip)
		{
			bTipWasInFront[SlotIndex] = false;
			continue;
		}

		const FVector Center = GetSlotLocalLocation(SlotIndex);
		const float SideDistance = FVector2D(Tip.Y - Center.Y, Tip.Z - Center.Z).Size();
		const float DistanceInFront = Tip.X - ButtonHeight;

		bool bInFront = false;
		bool bPressing = false;
		if (SideDistance <= ButtonRadius + FingertipRadius)
		{
			bInFront = DistanceInFront > 0.0f && DistanceInFront <= HoverDistance;
			bPressing = DistanceInFront <= 0.0f && DistanceInFront >= -PokeDepth;
		}

		if (bInFront || bPressing)
		{
			NewHovered = SlotIndex;
		}

		// A press is a fingertip crossing the button face from the front
		if (bPressing && bTipWasInFront[SlotIndex] && TimeSincePressed[SlotIndex] >= PressCooldown)
		{
			PressSlot(SlotIndex);
			// The press may have closed the menu or rebuilt the slots
			if (!bOpen)
			{
				return;
			}
		}
		bTipWasInFront[SlotIndex] = bInFront;
	}

	if (NewHovered != HoveredSlot)
	{
		HoveredSlot = NewHovered;
		RefreshVisuals();
	}
}

void AWristMenu::UpdateLabelFacing()
{
	FVector CameraLocation;
	if (!HandPointerUtils::GetCameraLocation(GetWorld(), CameraLocation))
	{
		return;
	}

	// Text components face +X; turn each toward the viewer so the text always reads correctly
	auto Face = [&CameraLocation](UTextRenderComponent* Text)
	{
		Text->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(Text->GetComponentLocation(), CameraLocation));
	};
	for (UTextRenderComponent* Label : ButtonLabels)
	{
		Face(Label);
	}
	Face(TitleText);
	Face(StatusText);
}

void AWristMenu::HandleDebugKeys()
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::M))
	{
		ToggleMenu();
	}
	if (PlayerController->WasInputKeyJustPressed(EKeys::Z) && History)
	{
		History->Undo();
	}
	if (PlayerController->WasInputKeyJustPressed(EKeys::Y) && History)
	{
		History->Redo();
	}

	if (bOpen && Page == EWristMenuPage::Products)
	{
		const FKey NumberKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
		{
			if (PlayerController->WasInputKeyJustPressed(NumberKeys[Index]))
			{
				PressSlot(FirstContentSlot + Index);
				break;
			}
		}
	}
}
