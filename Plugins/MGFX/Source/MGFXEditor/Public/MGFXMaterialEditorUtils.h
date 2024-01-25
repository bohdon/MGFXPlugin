// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories.h"

class IMGFXMaterialLayerParentInterface;
class UMGFXMaterial;
class UMGFXMaterialLayer;


/**
 * Handles converting exported text into MGFXMaterialLayer objects.
 */
class FMGFXMaterialLayerObjectTextFactory : public FCustomizableTextObjectFactory
{
public:
	FMGFXMaterialLayerObjectTextFactory()
		: FCustomizableTextObjectFactory(GWarn)
	{
	}

	/** Map of new objects by name */
	TMap<FName, UMGFXMaterialLayer*> NewLayersMap;

	virtual bool CanCreateClass(UClass* ObjectClass, bool& bOmitSubObjs) const override;
	virtual void ProcessConstructedObject(UObject* CreatedObject) override;
};


class MGFXEDITOR_API FMGFXMaterialEditorUtils
{
public:
	/** Return a unique layer name considering all layers in a material. */
	static FString MakeUniqueLayerName(const FString& Name, const UMGFXMaterial* InMaterial);

	/** Copy one or more layers to text. */
	static FString CopyLayers(const TArray<UMGFXMaterialLayer*>& LayersToCopy);

	/** Past layers into a container. */
	static TArray<UMGFXMaterialLayer*> PasteLayers(UMGFXMaterial* MGFXMaterial, const FString& TextToImport,
	                                               IMGFXMaterialLayerParentInterface* Container, bool& bSuccess);

	/** Export an array of layers to a text string. */
	static FString ExportLayersToText(const TArray<UMGFXMaterialLayer*>& LayersToExport);

	/** Import an array of layers from a text string. */
	static void ImportLayersFromText(UMGFXMaterial* MGFXMaterial, const FString& ImportText, TArray<UMGFXMaterialLayer*>& ImportedLayers);

	/** Return only the topmost layers, i.e. remove any layers that are children of other layers in the list. */
	static TArray<UMGFXMaterialLayer*> GetTopmostLayers(const TArray<UMGFXMaterialLayer*>& Layers);

private:
	/** Return a unique subobject name. */
	static FName GetUniqueName(UObject* Outer, const FName& Name);
};
