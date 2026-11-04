using UnrealBuildTool;

public class TaskValidatorEditor : ModuleRules
{
	public TaskValidatorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Editor-only: adds a "Task Validator" tab to the Place Actors panel.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "PlacementMode", "Slate", "SlateCore",
			"TaskValidator", "TaskValidatorDemo" });
	}
}
