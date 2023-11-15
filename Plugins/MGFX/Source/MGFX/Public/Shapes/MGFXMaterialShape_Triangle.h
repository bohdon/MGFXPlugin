﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Triangle.generated.h"


/**
 * A triangle that supports rounded corners.
 */
UCLASS(DisplayName = "Triangle")
class MGFX_API UMGFXMaterialShape_Triangle : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Triangle();

	/** The size of the triangle. */
	UPROPERTY(EditAnywhere, Category = "Triangle")
	float Size = 100.f;

	/** The corner radius of the triangle. */
	UPROPERTY(EditAnywhere, Category = "Triangle")
	float CornerRadius = 0.f;

	virtual FString GetShapeName() const override { return TEXT("Triangle"); }

#if WITH_EDITORONLY_DATA
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};