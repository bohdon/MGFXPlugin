// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialShape.h"
#include "MGFXMaterialShape_Rect.generated.h"


/**
 * A rectangle that supports rounded corners.
 */
UCLASS(DisplayName = "Rect")
class MGFX_API UMGFXMaterialShape_Rect : public UMGFXMaterialShape
{
	GENERATED_BODY()

public:
	UMGFXMaterialShape_Rect();

	/** The size of the rectangle. */
	UPROPERTY(EditAnywhere, Category = "Rect")
	FVector2f Size = FVector2f(100, 100);

	/** The radius of the rectangles corners. */
	UPROPERTY(EditAnywhere, Category = "Rect")
	float CornerRadius = 0.f;

	virtual FString GetShapeName() const override { return TEXT("Rect"); }
	virtual bool HasBounds() const override { return true; }
	virtual FBox2D GetBounds() const override;

#if WITH_EDITORONLY_DATA
	virtual TArray<FMGFXMaterialShapeInput> GetInputs() const override;
#endif
};
