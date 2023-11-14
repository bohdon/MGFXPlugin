// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "MGFXMaterial.generated.h"


USTRUCT(BlueprintType)
struct FMGFXShapeTransform2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Scale = FVector2f(1, 1);
};

USTRUCT(BlueprintType)
struct MGFX_API FMGFXMaterialShape
{
	GENERATED_BODY()

	/** The unique name of this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMGFXShapeTransform2D Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ShapeType;

	// TODO: needs to be unique per shape
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2f Size = FVector2f(100, 100);

	FString GetName(int32 LayerIdx) const;
};


/**
 * A motion graphics material.
 */
UCLASS(BlueprintType)
class MGFX_API UMGFXMaterial : public UObject
{
	GENERATED_BODY()

public:
	UMGFXMaterial();

	/** The base size in pixels of the canvas, for determining shape locations and sizes. */
	UPROPERTY(EditAnywhere, Category = "Canvas")
	FVector2D BaseCanvasSize;

	/** The generated material asset being edited. */
	UPROPERTY(EditAnywhere, Category = "Advanced")
	TObjectPtr<UMaterial> GeneratedMaterial;

	// TODO: use instanced struct?
	/** The shape layers in this material. */
	UPROPERTY(EditAnywhere, Meta = (TitleProperty = "Name"), Category = "Shapes")
	TArray<FMGFXMaterialShape> ShapeLayers;
};
