// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/TransformCalculus2D.h"
#include "MGFXMaterialTypes.generated.h"


/**
 * Possible merge operations for SDF shape layers.
 */
UENUM(BlueprintType)
enum class EMGFXShapeMergeOperation : uint8
{
	/** Don't merge this shape with another. */
	None,
	/** Combine the shape with the shapes below. */
	Union,
	/** Subtract the shape from the shapes below. */
	Subtraction,
	/** Intersect the shape with the shapes below. */
	Intersection,
};


/**
 * Possible merge operations for layer visuals.
 */
UENUM(BlueprintType)
enum class EMGFXLayerMergeOperation : uint8
{
	/** A + B * (1-a). The default operation, overlays A on top of B. */
	Over,
	/** A + B */
	Add,
	/** B - A */
	Subtract,
	/** A * B */
	Multiply,
	/** Ab (A with the alpha of B) */
	In,
	/** A(1-b) (A with the inverse alpha of B) */
	Out,
	/** Ba (B with the alpha of a). The reverse of the In operation. */
	Mask,
	/** B(1-a) (B with the inverse alpha of a). The reverse of the Out operation. */
	Stencil,
};


USTRUCT(BlueprintType)
struct MGFX_API FMGFXShapeTransform2D
{
	GENERATED_BODY()

	FMGFXShapeTransform2D()
	{
	}

	FMGFXShapeTransform2D(const FTransform2D& InTransform);

	/** When true, all values will be setup as animatable parameters, otherwise they may be optimized out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAnimatable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Location = FVector2f(0.f, 0.f);

	/** The rotation of the shape in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rotation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Scale = FVector2f(1, 1);

	FTransform2D ToTransform2D() const;

	/** Return true if the location, rotation, and scale are equal. */
	bool IsEqual(const FMGFXShapeTransform2D& Other) const
	{
		return Location == Other.Location &&
			Rotation == Other.Rotation &&
			Scale == Other.Scale;
	}
};
