﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"

#include "MaterialDomain.h"
#include "Brushes/SlateColorBrush.h"


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

#if WITH_EDITOR
	// fix up incorrect outers
	TArray<TObjectPtr<UMGFXMaterialLayer>> AllLayers;
	GetAllLayers(AllLayers);
	for (TObjectPtr<UMGFXMaterialLayer> Layer : AllLayers)
	{
		if (Layer->GetOuter() != this)
		{
			Layer->Rename(nullptr, this);
		}
	}
#endif
}
