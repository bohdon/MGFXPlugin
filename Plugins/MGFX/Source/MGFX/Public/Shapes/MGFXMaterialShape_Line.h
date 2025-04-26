// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Line.generated.h"


/**
 * A line segment with two points.
 */
UCLASS(DisplayName = "Line")
class MGFX_API UMGFXMaterialShape_Line : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Line();

	UPROPERTY(EditAnywhere, Category = "Line")
	FVector2f PointA = FVector2f(0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Line")
	FVector2f PointB = FVector2f(100.f, 100.f);

#if WITH_EDITOR
	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
