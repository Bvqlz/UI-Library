/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "ControllerBox.h"
#include "TrainLocomotive.h"
#include "CowCar.h"
#include "IXRTrackingSystem.h"
#include "Engine/Engine.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "HandsVisualizationSwitcher.h"
#include <Components/StaticMeshComponent.h>

#define LOCTEXT_NAMESPACE "ControllerBox"

namespace
{
	struct FControllerBoxButtonInfo
	{
		const TCHAR* ComponentName;
		/** The anchor component in BP_ControllerBox, placed from the FBX. */
		const TCHAR* AnchorName;
		FText Label;
	};

	/** Indexed by EControllerBoxButton. */
	const TArray<FControllerBoxButtonInfo>& ButtonInfos()
	{
		static const TArray<FControllerBoxButtonInfo> Infos = {
			{ TEXT("SmokeButton"), TEXT("SmokeButtonAnchor"), LOCTEXT("Smoke", "Smoke") },
			{ TEXT("WhistleButton"), TEXT("WhistleButtonAnchor"), LOCTEXT("Whistle", "Whistle") },
			{ TEXT("MooCowButton"), TEXT("MooCowButtonAnchor"), LOCTEXT("Moo", "Moo") },
			{ TEXT("HandStyleButton"), TEXT("HandStyleButtonAnchor"), LOCTEXT("Hands", "Hands") },
			{ TEXT("ReverseButton"), TEXT("ReverseButtonAnchor"), LOCTEXT("Reverse", "Reverse") },
			{ TEXT("SpeedUpButton"), TEXT("SpeedUpButtonAnchor"), LOCTEXT("Faster", "Faster") },
			{ TEXT("SlowDownButton"), TEXT("SlowDownButtonAnchor"), LOCTEXT("Slower", "Slower") },
			{ TEXT("StartStopButton"), TEXT("StartStopButtonAnchor"), LOCTEXT("StartStop", "Start / Stop") },
		};
		static_assert(static_cast<int32>(EControllerBoxButton::Count) == 8, "Update ButtonInfos with EControllerBoxButton");
		return Infos;
	}
}

const float AControllerBox::TotalFollowDuration = 3.0f;
const float AControllerBox::HmdMovementThreshold = 30.0f;

AControllerBox::AControllerBox()
{
	PrimaryActorTick.bCanEverTick = true;

	MaxDistanceFromHMDToLastPos = 30.0f;
	MinDistanceToHMD = 5.0f;
	MinDepthToHMD = 5.0f;
	LastMovedToPos = FVector::ZeroVector;
	PanelOffsetFromHMD = FVector(30.0f, 0.0f, -40.0f);
}

void AControllerBox::CreateButtons()
{
	// One Interaction SDK poke button per action, on the anchor placed from ControllerBox.fbx so the layout is kept.
	// Created at runtime because the anchors are Blueprint components.
	const TArray<FControllerBoxButtonInfo>& Infos = ButtonInfos();
	Buttons.Reset();
	Buttons.SetNum(Infos.Num());

	for (int32 Index = 0; Index < Infos.Num(); ++Index)
	{
		const FControllerBoxButtonInfo& Info = Infos[Index];
		USceneComponent* Anchor = Cast<USceneComponent>(GetDefaultSubobjectByName(FName(Info.AnchorName)));
		if (!Anchor)
		{
			UE_LOG(LogTemp, Error, TEXT("Controller box found no %s, so it has no %s"), Info.AnchorName, Info.ComponentName);
			continue;
		}

		UIsdkRoundedButtonComponent* Button = NewObject<UIsdkRoundedButtonComponent>(this, FName(Info.ComponentName));
		Button->ButtonSize = ButtonSize;
		Button->SetupAttachment(Anchor);
		Button->SetRelativeRotation(ButtonRotationOffset);
		Button->RegisterComponent();
		Button->SetLabelText(Info.Label);
		Button->SetLabelSize(ButtonLabelSize);
		AddInstanceComponent(Button);
		Buttons[Index] = Button;
	}
}

UIsdkRoundedButtonComponent* AControllerBox::GetButton(EControllerBoxButton Button) const
{
	const int32 Index = static_cast<int32>(Button);
	return Buttons.IsValidIndex(Index) ? Buttons[Index].Get() : nullptr;
}

void AControllerBox::FindLocomotive()
{
	if (!IsValid(Locomotive))
	{
		Locomotive = (ATrainLocomotive*)UGameplayStatics::GetActorOfClass(GetWorld(),
			ATrainLocomotive::StaticClass());
		if (!IsValid(Locomotive))
		{
			UE_LOG(LogTemp, Error, TEXT("Controller box found no locomotive actor!"));
		}
	}
}

void AControllerBox::FindCowCar()
{
	if (!IsValid(CowCar))
	{
		CowCar = (ACowCar*)UGameplayStatics::GetActorOfClass(GetWorld(),
			ACowCar::StaticClass());
		if (!IsValid(CowCar))
		{
			UE_LOG(LogTemp, Error, TEXT("Controller box found no cow car actor!"));
		}
	}
}

void AControllerBox::FindHandsVisualizationSwitcher()
{
	if (!IsValid(HandsVisSwitcher))
	{
		HandsVisSwitcher = (AHandsVisualizationSwitcher*)
			UGameplayStatics::GetActorOfClass(GetWorld(),
				AHandsVisualizationSwitcher::StaticClass());
		if (!IsValid(HandsVisSwitcher))
		{
			UE_LOG(LogTemp, Error, TEXT("Controller box found no hands switcher!"));
		}
	}
}

void AControllerBox::BeginPlay()
{
	Super::BeginPlay();

	bIsLerpingToHMD = false;

	CreateButtons();
	HookUpButtonEvents();
}

void AControllerBox::HookUpButtonEvents()
{
	for (UIsdkRoundedButtonComponent* Button : Buttons)
	{
		if (Button && Button->GetPokeInteractable())
		{
			Button->GetPokeInteractable()->GetInteractionPointerEventDelegate().AddUniqueDynamic(
				this, &AControllerBox::HandleButtonPointerEvent);
		}
	}

	// useful for testing button actions on PC
	// in case hand tracking doesn't work in editor
#if WITH_EDITOR
	EnableInput(GetWorld()->GetFirstPlayerController());
	InputComponent->BindAction("ActionTrainStartStop",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::StartStopTrain);

	InputComponent->BindAction("ActionTrainSmoke",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::TrainBlowSmoke);

	InputComponent->BindAction("ActionCowMoo",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::CowMoo);

	InputComponent->BindAction("ActionTrainWhistle",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::TrainBlowWhistle);

	InputComponent->BindAction("ActionTrainSpeedUp",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::TrainSpeedUp);

	InputComponent->BindAction("ActionTrainSlowDown",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::TrainSlowDown);

	InputComponent->BindAction("ActionTrainReverse",
		EInputEvent::IE_Pressed, this,
		&AControllerBox::TrainReverse);
#endif
}

void AControllerBox::HandleButtonPointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
{
	// Select is the press. Hover, Unselect and the rest only drive the button's own visuals.
	if (PointerEvent.Type != EIsdkPointerEventType::Select)
	{
		return;
	}
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		if (Buttons[Index] && Buttons[Index]->GetPokeInteractable() == PointerEvent.Interactable.GetObject())
		{
			PressButton(static_cast<EControllerBoxButton>(Index));
			return;
		}
	}
}

void AControllerBox::PressButton(EControllerBoxButton Button)
{
	switch (Button)
	{
	case EControllerBoxButton::Smoke:     TrainBlowSmoke(); break;
	case EControllerBoxButton::Whistle:   TrainBlowWhistle(); break;
	case EControllerBoxButton::MooCow:    CowMoo(); break;
	case EControllerBoxButton::HandStyle:
		FindHandsVisualizationSwitcher();
		if (IsValid(HandsVisSwitcher))
		{
			HandsVisSwitcher->SwitchHandsVisualization();
		}
		break;
	case EControllerBoxButton::Reverse:   TrainReverse(); break;
	case EControllerBoxButton::SpeedUp:   TrainSpeedUp(); break;
	case EControllerBoxButton::SlowDown:  TrainSlowDown(); break;
	case EControllerBoxButton::StartStop: StartStopTrain(); break;
	default: break;
	}
}

void AControllerBox::StartStopTrain()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->StartStopTrainStateChanged();
	}
}

void AControllerBox::TrainBlowSmoke()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->BlowLotsOfSmoke();
	}
}

void AControllerBox::CowMoo()
{
	FindCowCar();
	if (IsValid(CowCar))
	{
		CowCar->GoMooCowGo();
	}
}

void AControllerBox::TrainBlowWhistle()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->WhistleButtonStateChanged();
	}
}

void AControllerBox::TrainSpeedUp()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->IncreaseSpeed();
	}
}

void AControllerBox::TrainSlowDown()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->DecreaseSpeed();
	}
}

void AControllerBox::TrainReverse()
{
	FindLocomotive();
	if (IsValid(Locomotive))
	{
		Locomotive->Reverse();
	}
}

void AControllerBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto CurrentHMDPosition = GetHMDPosition();
	float DistanceFromHMDPosToLastPos = FVector::Dist(CurrentHMDPosition,
		LastMovedToPos);
	float HeadMovementSpeed = FVector::Dist(CurrentHMDPosition, PrevPos) / DeltaTime;

	FVector CurrentLocation = GetActorLocation();
	FVector HMDToMyLocation = CurrentLocation - CurrentHMDPosition;
	float CurrDistanceFromHMD = HMDToMyLocation.Size();

	/**
	 * 1) Check if HMD moves too far from its original stable position. -OR-
	 * 2) Check if HMD is too close to controller box. -OR-
	 * 3) Check if depth is too far relative to controller box. -AND-
	 * 4) Make sure head stabilizes and isn't moving too fast currently.
	 * -AND-
	 * 5) We are not lerping
	 */
	if (((DistanceFromHMDPosToLastPos > MaxDistanceFromHMDToLastPos) || (MinDistanceToHMD > CurrDistanceFromHMD) || (MinDepthToHMD > HMDToMyLocation.X)) && (HeadMovementSpeed < HmdMovementThreshold)
		&& !bIsLerpingToHMD)
	{
		LastMovedToPos = CurrentHMDPosition;
		bIsLerpingToHMD = true;
		LerpToPosition(TotalFollowDuration, TotalFollowDuration,
			CurrentLocation, GetIdealAnchorPosition());
	}

	PrevPos = CurrentHMDPosition;
}

FVector AControllerBox::GetIdealAnchorPosition()
{
	FVector HMDPosition = GetHMDPosition();
	return HMDPosition + PanelOffsetFromHMD;
}

FVector AControllerBox::GetHMDPosition()
{
	FRotator DeviceRotation;
	FVector DevicePosition;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!IsValid(PlayerPawn))
	{
		return FVector::ZeroVector;
	}
	FTransform PlayerTransform = PlayerPawn->GetTransform();
	FVector TransformedPos = PlayerTransform.TransformPosition(DevicePosition);
	return TransformedPos;
}

#undef LOCTEXT_NAMESPACE
