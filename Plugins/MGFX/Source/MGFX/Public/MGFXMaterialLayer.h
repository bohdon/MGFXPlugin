// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialTypes.h"
#include "UObject/Object.h"
#include "MGFXMaterialLayer.generated.h"

class UMGFXMaterialShape;


/**
 * A layer within a UMGFXMaterial.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class MGFX_API UMGFXMaterialLayer : public UObject
{
	GENERATED_BODY()

public:
	UMGFXMaterialLayer();

	/** The display name of this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FString Name;

	/** The index of the layer among all layers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layer")
	int32 Index;

	/** The merge operation to use for visuals in this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	EMGFXLayerMergeOperation MergeOperation;

	/** The layer's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FMGFXShapeTransform2D Transform;

	/** The shape to create for this layer. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Shape")
	TObjectPtr<UMGFXMaterialShape> Shape;

	UPROPERTY(EditAnywhere, Instanced, Meta = (TitleProperty = "Name"), Category = "Children")
	TArray<TObjectPtr<UMGFXMaterialLayer>> Children;

	/** Return the accumulated transform of this layer. */
	FTransform2D GetTransform() const;

	/** Return the accumulated transform of the parent layer. */
	FTransform2D GetParentTransform() const;

	/** Return true if this layer has a shape with finite bounds. */
	bool HasBounds() const;

	/** Return the local bounds of this layer's shape. */
	FBox2D GetBounds() const;

	/** Add a new child layer. */
	void AddChild(UMGFXMaterialLayer* Child);

	UMGFXMaterialLayer* GetParent() const { return Parent; }

	void SetParent(UMGFXMaterialLayer* NewParent);

	virtual void PostLoad() override;

protected:
	/** The parent layer, if any. */
	UPROPERTY()
	TObjectPtr<UMGFXMaterialLayer> Parent;
};
