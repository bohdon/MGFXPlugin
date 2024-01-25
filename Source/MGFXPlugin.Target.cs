// Copyright Bohdon Sayre, All Rights Reserved.

using UnrealBuildTool;

public class MGFXPluginTarget : TargetRules
{
	public MGFXPluginTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		ExtraModuleNames.AddRange(new string[]
		{
			"MGFXPlugin"
		});
	}
}