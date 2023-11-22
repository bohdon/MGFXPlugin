// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialTypes.h"


FMGFXShapeTransform2D::FMGFXShapeTransform2D(const FTransform2D& InTransform)
{
	Location = FVector2f(InTransform.GetTranslation());
	Rotation = -FMath::RadiansToDegrees(InTransform.GetMatrix().GetRotationAngle());
	Scale = InTransform.GetMatrix().GetScale().GetVector();
}

FTransform2D FMGFXShapeTransform2D::ToTransform2D() const
{
	return ::Concatenate(
		FScale2D(Scale),
		FQuat2D(FMath::DegreesToRadians(Rotation)),
		FVector2D(Location));
}
