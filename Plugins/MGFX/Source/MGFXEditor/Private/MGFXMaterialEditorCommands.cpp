// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditorCommands.h"

#define LOCTEXT_NAMESPACE "MGFXMaterialEditorCommands"

void FMGFXMaterialEditorCommands::RegisterCommands()
{
	UI_COMMAND(RegenerateMaterial, "Regenerate", "Regenerate the target material", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
