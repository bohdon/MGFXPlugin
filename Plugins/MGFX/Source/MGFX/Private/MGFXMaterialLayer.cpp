// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialLayer.h"

#include "Shapes/MGFXMaterialShape.h"


UMGFXMaterialLayer::UMGFXMaterialLayer()
	: Name(TEXT("Layer")),
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

void UMGFXMaterialLayer::AddChild(UMGFXMaterialLayer* Child, int32 Index)
{
	check(Child);
	check(!Children.Contains(Child));

	Modify();

	if (Children.IsValidIndex(Index))
	{
		Children.Insert(Child, Index);
	}
	else
	{
		Children.Add(Child);
	}

	Child->SetParent(this);
}

void UMGFXMaterialLayer::RemoveChild(UMGFXMaterialLayer* Child)
{
	if (Children.Contains(Child))
	{
		Modify();
		Children.Remove(Child);
		Child->SetParent(nullptr);
	}
}

void UMGFXMaterialLayer::ReorderChild(UMGFXMaterialLayer* Child, int32 NewIndex)
{
	if (Children.Contains(Child))
	{
		Modify();
		Children.Remove(Child);
		Children.Insert(Child, NewIndex);
	}
}

UMGFXMaterialLayer* UMGFXMaterialLayer::GetChild(int32 Index) const
{
	if (Children.IsValidIndex(Index))
	{
		return Children[Index];
	}
	return nullptr;
}

void UMGFXMaterialLayer::SetParent(UMGFXMaterialLayer* NewParent)
{
	Modify();
	Parent = NewParent;
}

int32 UMGFXMaterialLayer::GetIndexInParent() const
{
	if (Parent)
	{
		return Parent->GetChildren().IndexOfByKey(this);
	}
	return INDEX_NONE;
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
		}
	}
#endif
}
