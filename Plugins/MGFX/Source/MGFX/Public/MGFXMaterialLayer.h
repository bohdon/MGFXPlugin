// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MGFXMaterialLayer.generated.h"

class UMGFXMaterialShape;


USTRUCT(BlueprintType)
struct FMGFXShapeTransform2D
{
	GENERATED_BODY()

	/** When true, all values will be setup as animatable parameters, otherwise they may be optimized out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAnimatable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Location = FVector2f(0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rotation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Scale = FVector2f(1, 1);
};


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

	/** The layer's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FMGFXShapeTransform2D Transform;

	/** The shape to create for this layer. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Shape")
	TObjectPtr<UMGFXMaterialShape> Shape;

	UPROPERTY(EditAnywhere, Instanced, Meta = (TitleProperty = "Name"), Category = "Children")
	TArray<TObjectPtr<UMGFXMaterialLayer>> Children;
};
