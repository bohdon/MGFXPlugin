// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape.h"

#include "Misc/DataValidation.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


#define LOCTEXT_NAMESPACE "MGFX"


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

void UMGFXMaterialShape::AddDefaultVisual()
{
	if (DefaultVisualsClass && Visuals.IsEmpty() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UMGFXMaterialShapeVisual* DefaultVisual = NewObject<UMGFXMaterialShapeVisual>(this, DefaultVisualsClass, NAME_None, RF_Public | RF_Transactional);
		Visuals.Add(DefaultVisual);
	}
}

#if WITH_EDITORONLY_DATA
UMaterialFunctionInterface* UMGFXMaterialShape::GetMaterialFunction() const
{
	return !MaterialFunction.IsNull() ? MaterialFunction.LoadSynchronous() : nullptr;
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape::GetInputs_BP_Implementation() const
{
	// TODO: automatically retrieve using material function inputs and matching blueprint property names
	return TArray<FMGFXMaterialShapeInput>();
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape::GetInputs() const
{
	return GetInputs_BP();
}
#endif

#if WITH_EDITOR
EDataValidationResult UMGFXMaterialShape::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (MaterialFunction.IsNull())
	{
		Context.AddError(LOCTEXT("MaterialFunctionNull", "MaterialFunction must be set"));
		Result = EDataValidationResult::Invalid;
	}

	if (ShapeName.IsEmpty())
	{
		Context.AddError(LOCTEXT("ShapeNameEmpty", "ShapeName must be set"));
		Result = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
