// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXEditorStyle.h"
#include "Framework/Commands/Commands.h"


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

	/** Apply changes and regenerate the target material. */
	TSharedPtr<FUICommandInfo> Apply;

	/** Set the canvas view scale to 50% */
	TSharedPtr<FUICommandInfo> ZoomTo50;

	/** Set the canvas view scale to 100% */
	TSharedPtr<FUICommandInfo> ZoomTo100;

	/** Set the canvas view scale to 200% */
	TSharedPtr<FUICommandInfo> ZoomTo200;

	/** Set the canvas view scale to 300% */
	TSharedPtr<FUICommandInfo> ZoomTo300;

	virtual void RegisterCommands() override;
};
