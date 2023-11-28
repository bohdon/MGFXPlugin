// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Cross.generated.h"


/**
 * A cross shape.
 */
UCLASS(DisplayName = "Cross")
class MGFX_API UMGFXMaterialShape_Cross : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Cross();

	/** The size of the cross. */
	UPROPERTY(EditAnywhere, Category = "Cross")
	float Size = 100.f;

	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;

#if WITH_EDITORONLY_DATA
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
