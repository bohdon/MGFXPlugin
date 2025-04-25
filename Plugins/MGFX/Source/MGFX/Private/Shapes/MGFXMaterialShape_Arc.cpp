// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Arc.h"

#include "MGFXMaterialFunctionHelpers.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Arc::UMGFXMaterialShape_Arc()
{
	ShapeName = TEXT("Arc");
	MaterialFunction = FMGFXMaterialFunctions::GetShape(ShapeName);
	DefaultVisualsClass = UMGFXMaterialShapeFill::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Arc::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Arc::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Width"), Width));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Sweep"), Sweep));

	return Result;
}
#endif
