using UnrealBuildTool;

public class TaskValidator : ModuleRules
{
	public TaskValidator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Core module stays free of XR dependencies so it can validate any interaction system.
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
	}
}
