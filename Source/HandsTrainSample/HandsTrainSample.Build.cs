/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

using UnrealBuildTool;

public class HandsTrainSample : ModuleRules
{
    public HandsTrainSample(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore",
        "OculusXRInput"});

        // The sample is a host environment. It uses two project plugins and neither knows about the other:
        //   HandTrackingTools: hand tracking tools, interactables, hand visuals, wrist UI
        //   TaskValidator:     study task timing, attempts, success and CSV logging
        // Only code in this module (the validator bridge and the sample's own tasks) connects them.
        PublicDependencyModuleNames.AddRange(new string[] { "HandTrackingTools", "TaskValidator" });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "HeadMountedDisplay", "OculusXRHMD", "XRBase" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
