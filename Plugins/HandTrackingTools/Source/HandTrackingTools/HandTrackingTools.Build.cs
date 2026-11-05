/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

using UnrealBuildTool;

public class HandTrackingTools : ModuleRules
{
    public HandTrackingTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public headers include the Meta XR hand component and the motion controller component,
        // so any module that includes them needs these modules too.
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "OculusXRInput", "HeadMountedDisplay" });

        PrivateDependencyModuleNames.AddRange(new string[] { "OculusXRHMD", "XRBase" });

        // Deliberately no dependency on the sample or on TaskValidator. The tools are reusable on their own.
    }
}
