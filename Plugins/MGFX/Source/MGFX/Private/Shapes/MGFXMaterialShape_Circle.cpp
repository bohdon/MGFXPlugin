﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Circle.h"

#include "MGFXMaterialFunctionHelpers.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Circle::UMGFXMaterialShape_Circle()
{
	ShapeName = TEXT("Circle");
	MaterialFunction = FMGFXMaterialFunctions::GetShape(ShapeName);
	DefaultVisualsClass = UMGFXMaterialShapeFill::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Circle::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Circle::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
