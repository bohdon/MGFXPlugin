// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Line.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Line::UMGFXMaterialShape_Line()
{
#if WITH_EDITORONLY_DATA
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Line.MF_MGFX_Shape_Line"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeStroke>(TEXT("DefaultStroke")));
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Line::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("PointA"), PointA));
	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("PointB"), PointB));

	return Result;
}
#endif
