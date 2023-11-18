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

FBox2D UMGFXMaterialShape_Line::GetBounds() const
{
	return FBox2D(FVector2D(PointA), FVector2D(PointA)) + FBox2D(FVector2D(PointB), FVector2D(PointB));
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
