// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"

#include "Shapes/MGFXMaterialShape.h"


FString FMGFXMaterialLayer_DEPRECATED::GetName(int32 LayerIdx) const
{
	const FString LayerBaseName = Shape ? Shape->GetShapeName() : TEXT("Layer");
	return Name.IsEmpty() ? FString::Printf(TEXT("%s%d"), *LayerBaseName, LayerIdx) : Name;
}

UMGFXMaterial::UMGFXMaterial()
	: BaseCanvasSize(256, 256)
{
	RootLayer = CreateDefaultSubobject<UMGFXMaterialLayer>(TEXT("RootLayer"));
	RootLayer->Name = TEXT("RootLayer");
}

void UMGFXMaterial::PostLoad()
{
	UObject::PostLoad();

	if (!Layers.IsEmpty())
	{
		for (const FMGFXMaterialLayer_DEPRECATED& Layer : Layers)
		{
			UMGFXMaterialLayer* NewLayer = NewObject<UMGFXMaterialLayer>(this, NAME_None, RF_Public | RF_Transactional);
			NewLayer->Name = Layer.Name;
			NewLayer->Transform = Layer.Transform;
			NewLayer->Shape = Layer.Shape;
			RootLayer->Children.Add(NewLayer);
		}

		Layers.Empty();
	}
}
