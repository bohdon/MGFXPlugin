// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Rect.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Rect::UMGFXMaterialShape_Rect()
{
#if WITH_EDITORONLY_DATA
	ShapeName = TEXT("Rect");
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Rect.MF_MGFX_Shape_Rect"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeFill>(TEXT("DefaultFill")));
}

FBox2D UMGFXMaterialShape_Rect::GetBounds() const
{
	const FVector2D HalfSize = FVector2D(Size * 0.5f);
	return FBox2D(-HalfSize, HalfSize);
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Rect::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Vector2(TEXT("Size"), Size));
	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("CornerRadius"), CornerRadius));

	return Result;
}
#endif
