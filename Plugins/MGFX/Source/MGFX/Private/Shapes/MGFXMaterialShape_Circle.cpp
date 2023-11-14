// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Circle.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Circle::UMGFXMaterialShape_Circle()
{
#if WITH_EDITORONLY_DATA
	MaterialFunction = FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Circle.MF_MGFX_Shape_Circle");
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeFill>(TEXT("DefaultFill")));
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Circle::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
