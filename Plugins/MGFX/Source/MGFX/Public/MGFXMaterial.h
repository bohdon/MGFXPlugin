﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialLayer.h"
#include "Materials/Material.h"
#include "Styling/SlateBrush.h"
#include "MGFXMaterial.generated.h"

class UMGFXMaterialShape;


/**
 * A motion graphics material.
 */
UCLASS(BlueprintType)
class MGFX_API UMGFXMaterial : public UObject,
                               public IMGFXMaterialLayerParentInterface
{
	GENERATED_BODY()

public:
	UMGFXMaterial();

	/** The domain of the material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	TEnumAsByte<EMaterialDomain> MaterialDomain;

	/** Determines how the material's color is blended with background colors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	TEnumAsByte<EBlendMode> BlendMode;

	/** The material property where the layers output should be connected. This is a temporary solution to allow choosing Emissive or Opacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	TEnumAsByte<EMaterialProperty> OutputProperty;

	/** A constant color to use when not outputting any layer content to EmissiveColor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", Meta = (EditCondition = "OutputProperty != MP_EmissiveColor"))
	FLinearColor DefaultEmissiveColor;

	/** The base size in pixels of the canvas, for determining shape locations and sizes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canvas")
	FVector2f BaseCanvasSize;

	/** Should the filter width be computed dynamically? If the material needs to be scaled up or down whilst preserving sharp edges, set this to true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canvas")
	bool bComputeFilterWidth = true;

	/**
	 * The constant filter width to use when bComputeFilterWidth is disabled.
	 * Larger values will result in softer edges and smoother lines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "!bComputeFilterWidth"), Category = "Canvas")
	float FixedFilterWidth = 1.f;

	/** Override the designer background for this asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (InlineEditConditionToggle), Category = "Canvas")
	bool bOverrideDesignerBackground;

	/** The background of the canvas for previewing at design time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "bOverrideDesignerBackground"), Category = "Canvas")
	FSlateBrush DesignerBackground;

	/**
	 * Don't optimize out any parameters to allow for animating every property.
	 * This can be expensive so use sparingly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	bool bAllAnimatable = false;

	/** The target material asset being edited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Advanced")
	TObjectPtr<UMaterial> Material;

	/** The root layers of the material. */
	UPROPERTY()
	TArray<TObjectPtr<UMGFXMaterialLayer>> RootLayers;

	/** Return a flat list of all layers in the material. */
	void GetAllLayers(TArray<UMGFXMaterialLayer*>& OutLayers) const;

	// IMGFXMaterialLayerParentInterface
	virtual const TArray<TObjectPtr<UMGFXMaterialLayer>>& GetLayers() const override { return RootLayers; }
	virtual TArray<TObjectPtr<UMGFXMaterialLayer>>& GetMutableLayers() override { return RootLayers; }

	virtual void PostLoad() override;
};
