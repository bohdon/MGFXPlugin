// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "MGFXMaterial.generated.h"


class UMGFXMaterialShape;

USTRUCT(BlueprintType)
struct FMGFXShapeTransform2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Scale = FVector2f(1, 1);
};


USTRUCT(BlueprintType)
struct MGFX_API FMGFXMaterialLayer
{
	GENERATED_BODY()

	/** The unique name of this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	/** The layer's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMGFXShapeTransform2D Transform;

	/** The shape to create for this layer. */
	UPROPERTY(EditAnywhere, Instanced)
	TObjectPtr<UMGFXMaterialShape> Shape;

	FString GetName(int32 LayerIdx) const;
};


/**
 * A motion graphics material.
 */
UCLASS(BlueprintType)
class MGFX_API UMGFXMaterial : public UObject
{
	GENERATED_BODY()

public:
	UMGFXMaterial();

	/** The base size in pixels of the canvas, for determining shape locations and sizes. */
	UPROPERTY(EditAnywhere, Category = "Canvas")
	FVector2f BaseCanvasSize;

	/**
	 * Should the filter width be computed dynamically? Creates sharper shapes when scaled to large sizes,
	 * but can lead to artifacts on thin lines and round edges.
	 */
	UPROPERTY(EditAnywhere, Category = "Canvas")
	bool bComputeFilterWidth = true;

	/**
	 * The scale applied to the computed filter width, or the constant filter width to use when bComputeFilterWidth is disabled.
	 * Larger values will result in softer edges and smoother lines.
	 */
	UPROPERTY(EditAnywhere, Category = "Canvas")
	float FilterWidthScale = 1.f;

	/** The generated material asset being edited. */
	UPROPERTY(EditAnywhere, Category = "Advanced")
	TObjectPtr<UMaterial> GeneratedMaterial;

	/** The layers in this material. */
	UPROPERTY(EditAnywhere, Meta = (TitleProperty = "Name"), Category = "Shapes")
	TArray<FMGFXMaterialLayer> Layers;
};
