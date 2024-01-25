// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Line.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Line::UMGFXMaterialShape_Line()
{
	ShapeName = TEXT("Line");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Line.MF_MGFX_Shape_Line"));
	DefaultVisualsClass = UMGFXMaterialShapeStroke::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Line::GetBounds() const
{
	return FBox2D(FVector2D(PointA), FVector2D(PointA)) + FBox2D(FVector2D(PointB), FVector2D(PointB));
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Line::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("PointA"), PointA));
	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("PointB"), PointB));

	return Result;
}
#endif
