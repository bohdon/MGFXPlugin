// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MGFXMaterialShapeVisual.generated.h"


UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class MGFX_API UMGFXMaterialShapeVisual : public UObject
{
	GENERATED_BODY()

public:
	/** Return the main color of this visual */
	virtual FLinearColor GetColor() const { return FLinearColor::White; }
};


UCLASS(DisplayName = "Fill")
class MGFX_API UMGFXMaterialShapeFill : public UMGFXMaterialShapeVisual
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Fill")
	FLinearColor Color = FLinearColor::White;

	virtual FLinearColor GetColor() const override { return Color; }
};


UCLASS(DisplayName = "Stroke")
class MGFX_API UMGFXMaterialShapeStroke : public UMGFXMaterialShapeVisual
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Stroke")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Stroke")
	float StrokeWidth = 3.f;

	virtual FLinearColor GetColor() const override { return Color; }
};
