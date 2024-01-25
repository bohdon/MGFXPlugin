// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialTypes.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "MGFXMaterialShape.generated.h"

class UMGFXMaterialShapeVisual;
class UMaterialFunctionInterface;


UENUM(BlueprintType)
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


USTRUCT(BlueprintType)
struct FMGFXMaterialShapeInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMGFXMaterialShapeInputType Type = EMGFXMaterialShapeInputType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Value = FLinearColor(0.f, 0.f, 0.f, 0.f);

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
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MGFX_API UMGFXMaterialShape : public UObject
{
	GENERATED_BODY()

public:
	/** The name of this shape. */
	UPROPERTY(EditDefaultsOnly, Category = "Shape|Editor")
	FString ShapeName;

	/** The operation to use when merging this shape with the one below. */
	UPROPERTY(EditAnywhere, Category = "Shape")
	EMGFXShapeMergeOperation ShapeMergeOperation;

	UPROPERTY(EditAnywhere, Instanced, Category = "Fill / Stroke")
	TArray<TObjectPtr<UMGFXMaterialShapeVisual>> Visuals;

	/** The default visual to create when creating a new instance of this shape. */
	UPROPERTY(Transient)
	TSubclassOf<UMGFXMaterialShapeVisual> DefaultVisualsClass;

	/** Return the user-facing name of this shape type. */
	virtual FString GetShapeName() const { return ShapeName; }

	/** Return the material function to use. */
	virtual UMaterialFunctionInterface* GetMaterialFunction() const;

	/** Add the default visual for this shape. */
	virtual void AddDefaultVisual();

#if WITH_EDITOR
	// TODO: move to SMGFXMaterialShape widgets defined for each shape...
	/** Return true if the shape has finite bounds. */
	virtual bool HasBounds() const { return false; }

	/** Return the local bounds of the shape */
	virtual FBox2D GetBounds() const;

	/** Return an array of all the inputs to use. */
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const;

	UFUNCTION(BlueprintNativeEvent, DisplayName = "GetInputs")
	TArray<FMGFXMaterialShapeInput> GetInputs_BP() const;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	/** The material function of this shape. */
	UPROPERTY(EditDefaultsOnly, Category = "Shape|Editor")
	TSoftObjectPtr<UMaterialFunctionInterface> MaterialFunction;
};
