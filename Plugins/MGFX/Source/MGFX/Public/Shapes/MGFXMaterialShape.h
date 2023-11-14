// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MGFXMaterialShape.generated.h"

class UMaterialFunctionInterface;

enum class EMGFXMaterialShapeInputType : uint8
{
	// a float value
	Float,
	// a vector2 value
	Vector2,
	// a vector3 value, stored as FLinearColor
	Vector3,
	// a vector4 value, stored as FLinearColor
	Vector4,
};

struct FMGFXMaterialShapeInput
{
	const FString Name = FString();
	EMGFXMaterialShapeInputType Type = EMGFXMaterialShapeInputType::Float;
	FLinearColor Value = FLinearColor(0.f, 0.f, 0.f, 0.f);

public:
	/** Create a new float input. */
	static FMGFXMaterialShapeInput Float(const FString& InName, float InValue);

	/** Create a new Vector2 input. */
	static FMGFXMaterialShapeInput Vector2(const FString& InName, FVector2f InValue);

	/** Create a new Vector3 input. */
	static FMGFXMaterialShapeInput Vector3(const FString& InName, FVector3f InValue);

	/** Create a new Vector4 input. */
	static FMGFXMaterialShapeInput Vector4(const FString& InName, FVector4f InValue);
};


/**
 * Base class for shapes that can be used in a UMGFXMaterial.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class MGFX_API UMGFXMaterialShape : public UObject
{
	GENERATED_BODY()

public:
	/** Return the user-facing name of this shape type. */
	virtual FString GetShapeName() const PURE_VIRTUAL(, return FString(););

#if WITH_EDITORONLY_DATA
	/** Return the material function to use. */
	virtual UMaterialFunctionInterface* GetMaterialFunction() const;

	/** Return an array of all the inputs to use. */
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const;

protected:
	/** The material function of this shape. */
	TSoftObjectPtr<UMaterialFunctionInterface> MaterialFunction;
#endif
};
