/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "HandPointerUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "OculusXRHandComponent.h"

namespace HandPointerUtils
{
	UOculusXRHandComponent* FindHand(const UWorld* World, EOculusXRHandType Type)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!PlayerPawn)
		{
			return nullptr;
		}

		TArray<UOculusXRHandComponent*> HandComponents;
		PlayerPawn->GetComponents<UOculusXRHandComponent>(HandComponents);
		for (UOculusXRHandComponent* HandComponent : HandComponents)
		{
			if (HandComponent->SkeletonType == Type)
			{
				return HandComponent;
			}
		}
		return nullptr;
	}

	bool GetBoneLocation(const UWorld* World, EOculusXRHandType Type, EOculusXRBone Bone, FVector& OutLocation)
	{
		UOculusXRHandComponent* Hand = FindHand(World, Type);
		if (!IsValid(Hand) || !Hand->GetSkinnedAsset() || !UOculusXRInputFunctionLibrary::IsHandPositionValid(Type))
		{
			return false;
		}

		const FName BoneName(*UOculusXRInputFunctionLibrary::GetBoneName(Bone));
		const int32 BoneIndex = Hand->GetBoneIndex(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			return false;
		}

		OutLocation = Hand->GetBoneTransform(BoneIndex).GetLocation();
		return true;
	}

	bool GetCameraLocation(const UWorld* World, FVector& OutLocation)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		if (!PlayerController || !PlayerController->PlayerCameraManager)
		{
			return false;
		}

		OutLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		return true;
	}

	bool GetRay(const UWorld* World, EOculusXRHandType RayHand, FVector& OutOrigin, FVector& OutDirection, bool& bOutFromHand)
	{
		FVector Tip;
		FVector Wrist;
		if (GetBoneLocation(World, RayHand, EOculusXRBone::Index_Tip, Tip) &&
			GetBoneLocation(World, RayHand, EOculusXRBone::Wrist_Root, Wrist))
		{
			const FVector Direction = (Tip - Wrist).GetSafeNormal();
			if (!Direction.IsNearlyZero())
			{
				OutOrigin = Tip;
				OutDirection = Direction;
				bOutFromHand = true;
				return true;
			}
		}

		bOutFromHand = false;
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		if (!PlayerController || !PlayerController->PlayerCameraManager)
		{
			return false;
		}

		OutOrigin = PlayerController->PlayerCameraManager->GetCameraLocation();
		OutDirection = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
		return true;
	}

	bool GetPinchDistance(const UWorld* World, EOculusXRHandType Type, float& OutDistance)
	{
		FVector ThumbTip;
		FVector IndexTip;
		if (!GetBoneLocation(World, Type, EOculusXRBone::Thumb_Tip, ThumbTip) ||
			!GetBoneLocation(World, Type, EOculusXRBone::Index_Tip, IndexTip))
		{
			return false;
		}

		OutDistance = FVector::Distance(ThumbTip, IndexTip);
		return true;
	}
}
