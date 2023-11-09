// Copyright Bohdon Sayre, All Rights Reserved.


#include "Factories/MGFXMaterialFactory.h"

#include "MGFXMaterial.h"


UMGFXMaterialFactory::UMGFXMaterialFactory()
{
	SupportedClass = UMGFXMaterial::StaticClass();
	bCreateNew = true;
}

UObject* UMGFXMaterialFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMGFXMaterial>(InParent, InClass, InName, Flags, Context);
}
