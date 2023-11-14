// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Rect.h"


UMGFXMaterialShape_Rect::UMGFXMaterialShape_Rect()
{
#if WITH_EDITORONLY_DATA
	MaterialFunction = FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Rect.MF_MGFX_Shape_Rect");
#endif
}


#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Rect::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	// rectangle size
	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("Size"), Size));

	// corner radius
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("CornerRadius"), CornerRadius));

	return Result;
}
#endif
