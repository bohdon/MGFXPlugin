// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "MGFXMaterial.h"
#include "AssetDefinition_MGFXMaterial.generated.h"


/**
 * Asset definition for UMGFXMaterial.
 */
UCLASS()
class MGFXEDITOR_API UAssetDefinition_MGFXMaterial : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UMGFXMaterial::StaticClass(); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(32, 121, 151)); }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = {EAssetCategoryPaths::Material};
		return Categories;
	}

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
