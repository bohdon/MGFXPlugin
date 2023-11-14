// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXEditorStyle.h"


/**
 * Defines commands for the MGFX Material Editor.
 */
class MGFXEDITOR_API FMGFXMaterialEditorCommands : public TCommands<FMGFXMaterialEditorCommands>
{
public:
	FMGFXMaterialEditorCommands()
		: TCommands<FMGFXMaterialEditorCommands>
		(
			TEXT("MGFXMaterialEditor"),
			NSLOCTEXT("Contexts", "MGFXMaterialEditor", "MGFX Material Editor"),
			NAME_None,
			FMGFXEditorStyle::StaticStyleSetName
		)
	{
	}

	TSharedPtr<FUICommandInfo> RegenerateMaterial;

	void RegisterCommands() override;
};
