// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Circle.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Circle::UMGFXMaterialShape_Circle()
{
#if WITH_EDITORONLY_DATA
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Circle.MF_MGFX_Shape_Circle"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeFill>(TEXT("DefaultFill")));
}

FBox2D UMGFXMaterialShape_Circle::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Circle::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
