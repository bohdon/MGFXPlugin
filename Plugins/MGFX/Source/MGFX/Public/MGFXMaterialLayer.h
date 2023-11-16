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
};
