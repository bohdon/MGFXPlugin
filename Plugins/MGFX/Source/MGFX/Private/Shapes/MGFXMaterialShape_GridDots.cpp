// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_GridDots.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_GridDots::UMGFXMaterialShape_GridDots()
{
#if WITH_EDITORONLY_DATA
	ShapeName = TEXT("GridDots");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_GridDots.MF_MGFX_Shape_GridDots"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeFill>(TEXT("DefaultFill")));
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_GridDots::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("Spacing"), Spacing));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
