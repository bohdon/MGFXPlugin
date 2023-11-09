// Copyright Bohdon Sayre, All Rights Reserved.


#include "AssetDefinitions/AssetDefinition_MGFXMaterial.h"


#define LOCTEXT_NAMESPACE "UAssetDefinition_MGFXMaterial"

FText UAssetDefinition_MGFXMaterial::GetAssetDisplayName() const
{
	return LOCTEXT("MGFXMaterial", "MGFX Material");
}

#undef LOCTEXT_NAMESPACE
