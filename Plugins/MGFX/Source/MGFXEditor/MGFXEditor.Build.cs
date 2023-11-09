using UnrealBuildTool;

public class MGFXEditor : ModuleRules
{
	public MGFXEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AssetDefinition",
			"Core",
			"MGFX",
			"MaterialEditor",
			"UnrealEd",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
		});
	}
}