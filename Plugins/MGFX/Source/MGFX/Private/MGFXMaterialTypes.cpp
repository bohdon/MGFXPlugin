// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialTypes.h"


FTransform2D FMGFXShapeTransform2D::ToTransform2D() const
{
	return ::Concatenate(
		FScale2D(Scale),
		FQuat2D(FMath::DegreesToRadians(Rotation)),
		FVector2D(Location));
}
