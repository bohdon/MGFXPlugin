// Copyright Bohdon Sayre, All Rights Reserved.


#include "Shapes/MGFXMaterialShape.h"

#include "Materials/MaterialFunctionInterface.h"
#include "Shapes/MGFXMaterialShapeVisual.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MGFXMaterialShape)


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

void UMGFXMaterialShape::AddDefaultVisual()
{
	if (DefaultVisualsClass && Visuals.IsEmpty() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UMGFXMaterialShapeVisual* DefaultVisual = NewObject<UMGFXMaterialShapeVisual>(this, DefaultVisualsClass, NAME_None, RF_Public | RF_Transactional);
		Visuals.Add(DefaultVisual);
	}
}

TSoftObjectPtr<UMaterialFunctionInterface> UMGFXMaterialShape::GetMaterialFunctionPtr() const
{
	return MaterialFunction;
}

UMaterialFunctionInterface* UMGFXMaterialShape::GetMaterialFunction() const
{
	const TSoftObjectPtr<UMaterialFunctionInterface>& FunctionPtr = GetMaterialFunctionPtr();
	return !FunctionPtr.IsNull() ? FunctionPtr.LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
FBox2D UMGFXMaterialShape::GetBounds() const
{
	return FBox2D(ForceInit);
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape::GetInputs() const
{
	return GetInputs_BP();
}

TArray<FMGFXMaterialShapeInput> UMGFXMaterialShape::GetInputs_BP_Implementation() const
{
	// TODO: automatically retrieve using material function inputs and matching blueprint property names
	return TArray<FMGFXMaterialShapeInput>();
}

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
