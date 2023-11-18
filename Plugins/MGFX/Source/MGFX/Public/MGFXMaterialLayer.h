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

	/** The merge operation to use for visuals in this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	EMGFXLayerMergeOperation MergeOperation;

	/** The layer's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FMGFXShapeTransform2D Transform;

	/** The shape to create for this layer. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Shape")
	TObjectPtr<UMGFXMaterialShape> Shape;

	/** Return the accumulated transform of this layer. */
	FTransform2D GetTransform() const;

	/** Return the accumulated transform of the parent layer. */
	FTransform2D GetParentTransform() const;

	/** Return true if this layer has a shape with finite bounds. */
	bool HasBounds() const;

	/** Return the local bounds of this layer's shape. */
	FBox2D GetBounds() const;

	/** Add a new child layer. */
	void AddChild(UMGFXMaterialLayer* Child, int32 Index = INDEX_NONE);

	/** Remove a child layer, and clear its parent. */
	void RemoveChild(UMGFXMaterialLayer* Child);

	/** Reorder a child layer. */
	void ReorderChild(UMGFXMaterialLayer* Child, int32 NewIndex);

	const TArray<UMGFXMaterialLayer*>& GetChildren() const { return Children; }

	int32 NumChildren() const { return Children.Num(); }

	bool HasChildren() const { return !Children.IsEmpty(); }

	UMGFXMaterialLayer* GetChild(int32 Index) const;

	UMGFXMaterialLayer* GetParent() const { return Parent; }

	void SetParent(UMGFXMaterialLayer* NewParent);

	int32 GetIndexInParent() const;

	virtual void PostLoad() override;

protected:
	UPROPERTY()
	TArray<TObjectPtr<UMGFXMaterialLayer>> Children;

	/** The parent layer, if any. */
	UPROPERTY()
	TObjectPtr<UMGFXMaterialLayer> Parent;
};
