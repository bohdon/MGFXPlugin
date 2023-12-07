// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Triangle.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Triangle::UMGFXMaterialShape_Triangle()
{
#if WITH_EDITORONLY_DATA
	ShapeName = TEXT("Triangle");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Triangle.MF_MGFX_Shape_Triangle"));
#endif

	DefaultVisualsClass = UMGFXMaterialShapeFill::StaticClass();
}

FBox2D UMGFXMaterialShape_Triangle::GetBounds() const
{
	const FVector2D HalfSize = FVector2D(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Triangle::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("CornerRadius"), CornerRadius));

	return Result;
}
#endif
