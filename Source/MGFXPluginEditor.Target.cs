// Copyright Bohdon Sayre, All Rights Reserved.

using UnrealBuildTool;

public class MGFXPluginEditorTarget : TargetRules
{
	public MGFXPluginEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		ExtraModuleNames.AddRange(new string[]
		{
			"MGFXPlugin",
		});
	}
}