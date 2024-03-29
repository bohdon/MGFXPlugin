﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_GridDots.generated.h"


/**
 * A grid of circular dots.
 */
UCLASS(DisplayName = "GridDots")
class MGFX_API UMGFXMaterialShape_GridDots : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_GridDots();

	/** The spacing between dots. */
	UPROPERTY(EditAnywhere, Category = "GridDots")
	FVector2f Spacing = FVector2f(10.f, 10.f);

	/** The size of each dot. */
	UPROPERTY(EditAnywhere, Category = "GridDots")
	float Size = 2.f;

#if WITH_EDITOR
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
