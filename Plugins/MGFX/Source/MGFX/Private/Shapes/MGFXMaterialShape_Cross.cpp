// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Cross.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Cross::UMGFXMaterialShape_Cross()
{
#if WITH_EDITORONLY_DATA
	ShapeName = TEXT("Cross");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Cross.MF_MGFX_Shape_Cross"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeStroke>(TEXT("DefaultStroke")));
}

FBox2D UMGFXMaterialShape_Cross::GetBounds() const
{
	const FVector2D HalfSize(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Cross::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
