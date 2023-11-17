// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape.h"


FMGFXMaterialShapeInput FMGFXMaterialShapeInput::Float(const FString& InName, float InValue)
{
	return FMGFXMaterialShapeInput(InName, EMGFXMaterialShapeInputType::Float, FLinearColor(InValue, 0.f, 0.f, 0.f));
}

FMGFXMaterialShapeInput FMGFXMaterialShapeInput::Vector2(const FString& InName, FVector2f InValue)
{
	return FMGFXMaterialShapeInput(InName, EMGFXMaterialShapeInputType::Vector2, FLinearColor(InValue.X, InValue.Y, 0.f, 0.f));
}

FMGFXMaterialShapeInput FMGFXMaterialShapeInput::Vector3(const FString& InName, FVector3f InValue)
{
	return FMGFXMaterialShapeInput(InName, EMGFXMaterialShapeInputType::Vector3, FLinearColor(InValue.X, InValue.Y, InValue.Z, 0.f));
}

FMGFXMaterialShapeInput FMGFXMaterialShapeInput::Vector4(const FString& InName, FVector4f InValue)
{
	return FMGFXMaterialShapeInput(InName, EMGFXMaterialShapeInputType::Vector4, FLinearColor(InValue.X, InValue.Y, InValue.Z, InValue.W));
}

FBox2D UMGFXMaterialShape::GetBounds() const
{
	return FBox2D(ForceInit);
}

#if WITH_EDITORONLY_DATA
UMaterialFunctionInterface* UMGFXMaterialShape::GetMaterialFunction() const
{
	check(!MaterialFunction.IsNull());
	return MaterialFunction.LoadSynchronous();
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape::GetInputs() const
{
	return TArray<FMGFXMaterialShapeInput>();
}
#endif
