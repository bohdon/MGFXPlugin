// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Pie.h"

#include "MGFXMaterialFunctionHelpers.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Pie::UMGFXMaterialShape_Pie()
{
	ShapeName = TEXT("Pie");
	MaterialFunction = FMGFXMaterialFunctions::GetShape(ShapeName);
	DefaultVisualsClass = UMGFXMaterialShapeFill::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Pie::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Pie::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Sweep"), Sweep));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("CornerRadius"), CornerRadius));

	return Result;
}
#endif
