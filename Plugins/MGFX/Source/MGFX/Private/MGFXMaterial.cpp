// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterial.h"


FString FMGFXMaterialShape::GetName(int32 LayerIdx) const
{
	// TODO: use polymorphic shapes
	const FString ShapeTypeName = ShapeType == 0 ? TEXT("Rect") : TEXT("Line");
	return Name.IsEmpty() ? FString::Printf(TEXT("%s%d"), *ShapeTypeName, LayerIdx) : Name;
}

UMGFXMaterial::UMGFXMaterial()
	: BaseCanvasSize(256, 256)
{
}
