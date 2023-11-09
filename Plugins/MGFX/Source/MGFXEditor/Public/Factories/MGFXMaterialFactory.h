// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MGFXMaterialFactory.generated.h"


/**
 * Factory for creating UMGFXMaterial assets.
 */
UCLASS()
class MGFXEDITOR_API UMGFXMaterialFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMGFXMaterialFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
