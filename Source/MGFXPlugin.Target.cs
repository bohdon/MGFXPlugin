// Copyright Bohdon Sayre, All Rights Reserved.

using UnrealBuildTool;

public class MGFXPluginTarget : TargetRules
{
	public MGFXPluginTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[]
		{
			"MGFXPlugin"
		});
	}
}