﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Triangle.h"

#include "MGFXMaterialFunctionHelpers.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Triangle::UMGFXMaterialShape_Triangle()
{
	ShapeName = TEXT("Triangle");
	MaterialFunction = FMGFXMaterialFunctions::GetShape(ShapeName);
	DefaultVisualsClass = UMGFXMaterialShapeFill::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Triangle::GetBounds() const
{
	const FVector2D HalfSize = FVector2D(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Triangle::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("CornerRadius"), CornerRadius));

	return Result;
}
#endif
