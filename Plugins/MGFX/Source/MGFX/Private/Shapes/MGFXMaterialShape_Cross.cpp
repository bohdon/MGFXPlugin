// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape_Cross.h"

#include "Shapes/MGFXMaterialShapeVisual.h"


UMGFXMaterialShape_Cross::UMGFXMaterialShape_Cross()
{
#if WITH_EDITORONLY_DATA
	MaterialFunction = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Shape_Cross.MF_MGFX_Shape_Cross"));
#endif

	Visuals.Add(CreateDefaultSubobject<UMGFXMaterialShapeStroke>(TEXT("DefaultStroke")));
}

#if WITH_EDITORONLY_DATA
TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape_Cross::GetInputs() const
{
	TArray<FMGFXMaterialShapeInput> Result;

	Result.Emplace(FMGFXMaterialShapeInput::Float(TEXT("Size"), Size));

	return Result;
}
#endif
