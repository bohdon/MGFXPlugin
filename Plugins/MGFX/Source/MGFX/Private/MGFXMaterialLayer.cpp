// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialLayer.h"

#include "Shapes/MGFXMaterialShape.h"


UMGFXMaterialLayer::UMGFXMaterialLayer()
	: Name(TEXT("Layer")),
	  Index(0),
	  MergeOperation(EMGFXLayerMergeOperation::Over)
{
}

FTransform2D UMGFXMaterialLayer::GetTransform() const
{
	return Transform.ToTransform2D().Concatenate(GetParentTransform());
}

FTransform2D UMGFXMaterialLayer::GetParentTransform() const
{
	return Parent ? Parent->GetTransform() : FTransform2D();
}

bool UMGFXMaterialLayer::HasBounds() const
{
	return Shape && Shape->HasBounds();
}

FBox2D UMGFXMaterialLayer::GetBounds() const
{
	return Shape ? Shape->GetBounds() : FBox2D(ForceInit);
}

void UMGFXMaterialLayer::AddChild(UMGFXMaterialLayer* Child)
{
	check(Child);
	check(!Children.Contains(Child));

	Children.Add(Child);
	Child->Parent = this;
}

void UMGFXMaterialLayer::SetParent(UMGFXMaterialLayer* NewParent)
{
	Parent = NewParent;
}

void UMGFXMaterialLayer::PostLoad()
{
	UObject::PostLoad();

#if WITH_EDITOR
	// fixup parent references
	for (UMGFXMaterialLayer* Child : Children)
	{
		if (Child->Parent.Get() != this)
		{
			Child->SetParent(this);
			Child->Modify();
		}
	}
#endif
}
