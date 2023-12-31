﻿using UnrealBuildTool;

public class MGFXEditor : ModuleRules
{
	public MGFXEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"ApplicationCore",
			"AssetDefinition",
			"Core",
			"InputCore",
			"MGFX",
			"MaterialEditor",
			"ToolMenus",
			"UnrealEd",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"RenderCore",
			"Slate",
			"SlateCore",
			"UMG",
		});
	}
}