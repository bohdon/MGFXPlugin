// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Cross.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Cross::UMGFXMaterialShape_Cross()
{
	ShapeName = TEXT("Cross");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Cross.MF_MGFX_Shape_Cross"));
	DefaultVisualsClass = UMGFXMaterialShapeStroke::StaticClass();
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape_Cross::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Cross::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
