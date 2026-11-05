using UnrealBuildTool;

public class TaskValidatorDemo : ModuleRules
{
	public TaskValidatorDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "TaskValidator", "HeadMountedDisplay", "XRBase" });
	}
}
