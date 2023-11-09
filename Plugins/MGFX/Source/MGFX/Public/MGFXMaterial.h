// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "MGFXMaterial.generated.h"


/**
 * A motion graphics material.
 */
UCLASS(BlueprintType)
class MGFX_API UMGFXMaterial : public UObject
{
	GENERATED_BODY()

public:
	UMGFXMaterial();

	/** The generated material asset being edited. */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterial> GeneratedMaterial;
};
