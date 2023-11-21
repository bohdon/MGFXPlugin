// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditorCommands.h"

#define LOCTEXT_NAMESPACE "MGFXMaterialEditorCommands"

void FMGFXMaterialEditorCommands::RegisterCommands()
{
	UI_COMMAND(RegenerateMaterial, "Regenerate", "Regenerate the target material", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(ZoomTo50, "50%", "Zoom to 50%", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ZoomTo100, "100%", "Zoom to 100%", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ZoomTo200, "200%", "Zoom to 200%", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ZoomTo300, "300%", "Zoom to 300%", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
