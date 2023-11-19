// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialTypes.h"
#include "UObject/Object.h"
#include "MGFXMaterialLayer.generated.h"

class UMGFXMaterialLayer;
class UMGFXMaterialShape;


UINTERFACE(Meta = (CannotImplementInterfaceInBlueprint))
class MGFX_API UMGFXMaterialLayerParentInterface : public UInterface
{
	GENERATED_BODY()
};

/** Interface for an object that can contain UMGFXMaterialLayers. */
class MGFX_API IMGFXMaterialLayerParentInterface
{
	GENERATED_BODY()

public:
	/** Add a new child layer. */
	virtual void AddLayer(UMGFXMaterialLayer* Child, int32 Index = INDEX_NONE);

	/** Remove a child layer, and clear its parent. */
	virtual void RemoveLayer(UMGFXMaterialLayer* Child);

	/** Reorder a child layer. */
	virtual void ReorderLayer(UMGFXMaterialLayer* Child, int32 NewIndex);

	/** Return true if this object has a child layer. */
	virtual bool HasLayer(const UMGFXMaterialLayer* Child) const;

	/** Return the index of a child layer. */
	virtual int32 GetLayerIndex(const UMGFXMaterialLayer* Child) const;

	virtual int32 NumLayers() const;

	virtual bool HasLayers() const;

	virtual UMGFXMaterialLayer* GetLayer(int32 Index) const;

	/** Return all child layers. */
	virtual const TArray<TObjectPtr<UMGFXMaterialLayer>>& GetLayers() const = 0;

protected:
	/** Return the layers array used to contain. */
	virtual TArray<TObjectPtr<UMGFXMaterialLayer>>& GetMutableLayers() = 0;
};


/**
 * A layer within a UMGFXMaterial.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class MGFX_API UMGFXMaterialLayer : public UObject,
                                    public IMGFXMaterialLayerParentInterface
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

	UMGFXMaterialLayer* GetParentLayer() const { return Parent; }

	IMGFXMaterialLayerParentInterface* GetParentContainer() const;

	void SetParentLayer(UMGFXMaterialLayer* NewParent);

	virtual void PostLoad() override;

	void GetAllLayers(TArray<TObjectPtr<UMGFXMaterialLayer>>& OutLayers) const;

	// IMGFXMaterialLayerParentInterface
	virtual const TArray<TObjectPtr<UMGFXMaterialLayer>>& GetLayers() const override { return Children; }
	virtual TArray<TObjectPtr<UMGFXMaterialLayer>>& GetMutableLayers() override { return Children; }

protected:
	UPROPERTY()
	TArray<TObjectPtr<UMGFXMaterialLayer>> Children;

	/** The parent layer, if any. */
	UPROPERTY()
	TObjectPtr<UMGFXMaterialLayer> Parent;
};
