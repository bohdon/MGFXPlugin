// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Circle.generated.h"


/**
 * A circle.
 */
UCLASS(DisplayName = "Circle")
class MGFX_API UMGFXMaterialShape_Circle : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Circle();

	/** The diameter of the circle. */
	UPROPERTY(EditAnywhere, Category = "Circle")
	float Size = 100.f;

#if WITH_EDITOR
	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
