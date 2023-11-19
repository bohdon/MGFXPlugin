// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"

#include "MaterialDomain.h"
#include "Shapes/MGFXMaterialShape.h"


FString FMGFXMaterialLayer_DEPRECATED::GetName(int32 LayerIdx) const
{
	const FString LayerBaseName = Shape ? Shape->GetShapeName() : TEXT("Layer");
	return Name.IsEmpty() ? FString::Printf(TEXT("%s%d"), *LayerBaseName, LayerIdx) : Name;
}

UMGFXMaterial::UMGFXMaterial()
	: MaterialDomain(MD_UI),
	  BlendMode(BLEND_Translucent),
	  OutputProperty(MP_EmissiveColor),
	  DefaultEmissiveColor(FLinearColor::White),
	  BaseCanvasSize(256, 256),
	  bOverrideDesignerBackground(false)
{
	DesignerBackground = FSlateColorBrush(FLinearColor(0.005f, 0.005f, 0.005f));
}

void UMGFXMaterial::GetAllLayers(TArray<TObjectPtr<UMGFXMaterialLayer>>& OutLayers) const
{
	OutLayers.Reset();

	for (TObjectPtr<UMGFXMaterialLayer> Layer : RootLayers)
	{
		OutLayers.Add(Layer);
		Layer->GetAllLayers(OutLayers);
	}
}

void UMGFXMaterial::PostLoad()
{
	UObject::PostLoad();

	if (RootLayer_DEPRECATED)
	{
		RootLayer_DEPRECATED->Name = FString("RootLayer");
		RootLayers.Add(RootLayer_DEPRECATED);
		RootLayer_DEPRECATED = nullptr;
		Modify();
	}
}
