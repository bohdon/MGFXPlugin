// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Arc.generated.h"

/**
 * An arc
 */
UCLASS(DisplayName = "Arc")
class MGFX_API UMGFXMaterialShape_Arc : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Arc();

	/** The diameter of the arc. */
	UPROPERTY(EditAnywhere, Category = "Arc", Meta = (ClampMin = 0))
	float Size = 100.f;

	/** The width of the arc. */
	UPROPERTY(EditAnywhere, Category = "Arc", Meta = (ClampMin = 0))
	float Width = 3.f;

	/** The sweep of the arc. */
	UPROPERTY(EditAnywhere, Category = "Arc", Meta = (ClampMin = 0, ClampMax = 1))
	float Sweep = 0.5f;

#if WITH_EDITOR
	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
