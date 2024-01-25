// Copyright Bohdon Sayre, All Rights Reserved.

using UnrealBuildTool;

public class MGFXPlugin : ModuleRules
{
	public MGFXPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}