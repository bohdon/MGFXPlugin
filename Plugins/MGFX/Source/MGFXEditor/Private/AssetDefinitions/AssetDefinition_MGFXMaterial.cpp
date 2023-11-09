// Copyright Bohdon Sayre, All Rights Reserved.


#include "AssetDefinitions/AssetDefinition_MGFXMaterial.h"

#include "MGFXEditorModule.h"


#define LOCTEXT_NAMESPACE "UAssetDefinition_MGFXMaterial"

FText UAssetDefinition_MGFXMaterial::GetAssetDisplayName() const
{
	return LOCTEXT("MGFXMaterial", "MGFX Material");
}

EAssetCommandResult UAssetDefinition_MGFXMaterial::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	FMGFXEditorModule* MGFXEditorModule = &FModuleManager::LoadModuleChecked<FMGFXEditorModule>("MGFXEditor");

	for (UMGFXMaterial* MGFXMaterial : OpenArgs.LoadObjects<UMGFXMaterial>())
	{
		MGFXEditorModule->CreateMGFXMaterialEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, MGFXMaterial);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
