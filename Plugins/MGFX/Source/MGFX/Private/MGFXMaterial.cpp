// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"

#include "Shapes/MGFXMaterialShape.h"


FString FMGFXMaterialLayer::GetName(int32 LayerIdx) const
{
	const FString LayerBaseName = Shape ? Shape->GetShapeName() : TEXT("Layer");
	return Name.IsEmpty() ? FString::Printf(TEXT("%s%d"), *LayerBaseName, LayerIdx) : Name;
}

UMGFXMaterial::UMGFXMaterial()
	: BaseCanvasSize(256, 256)
{
}
