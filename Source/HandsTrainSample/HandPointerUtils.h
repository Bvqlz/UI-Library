/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusXRInputFunctionLibrary.h"

class UOculusXRHandComponent;
class UWorld;

/** Small helpers shared by the wrist menu and the placement controller. */
namespace HandPointerUtils
{
	/** The first player pawn's hand component of the given type, or null. */
	UOculusXRHandComponent* FindHand(const UWorld* World, EOculusXRHandType Type);

	/** World location of a hand bone. False if the hand has no valid tracking or the bone is missing. */
	bool GetBoneLocation(const UWorld* World, EOculusXRHandType Type, EOculusXRBone Bone, FVector& OutLocation);

	/** World location of the player camera. */
	bool GetCameraLocation(const UWorld* World, FVector& OutLocation);

	/**
	 * Pointing ray of a hand: from the index fingertip along the wrist-to-fingertip direction.
	 * Falls back to the camera's gaze when the hand is not tracked, so the editor works without a headset.
	 */
	bool GetRay(const UWorld* World, EOculusXRHandType RayHand, FVector& OutOrigin, FVector& OutDirection, bool& bOutFromHand);

	/** Thumb-tip to index-tip distance in cm. Returns false if either fingertip is untracked. */
	bool GetPinchDistance(const UWorld* World, EOculusXRHandType Type, float& OutDistance);
}
