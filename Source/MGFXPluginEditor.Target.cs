// Copyright Bohdon Sayre, All Rights Reserved.

using UnrealBuildTool;

public class MGFXPluginEditorTarget : TargetRules
{
	public MGFXPluginEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[]
		{
			"MGFXPlugin",
		});
	}
}