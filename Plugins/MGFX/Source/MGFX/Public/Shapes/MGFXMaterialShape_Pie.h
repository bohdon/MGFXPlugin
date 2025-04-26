// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Pie.generated.h"

/**
 * A pie
 */
UCLASS(DisplayName = "Pie")
class MGFX_API UMGFXMaterialShape_Pie : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Pie();

	/** The diameter of the circle. */
	UPROPERTY(EditAnywhere, Category = "Pie", Meta = (ClampMin = 0))
	float Size = 100.f;

	/** The sweep of the pie. */
	UPROPERTY(EditAnywhere, Category = "Pie", Meta = (ClampMin = 0, ClampMax = 1))
	float Sweep = 0.75f;

	/** The corner radius of the pie tips. */
	UPROPERTY(EditAnywhere, Category = "Pie")
	float CornerRadius = 0.f;

#if WITH_EDITOR
	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
