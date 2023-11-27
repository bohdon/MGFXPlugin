// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"

#include "MaterialDomain.h"


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
