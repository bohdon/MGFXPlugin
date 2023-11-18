// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialLayer.h"

#include "Shapes/MGFXMaterialShape.h"


// IMGFXMaterialLayerContainerInterface
// ------------------------------------

void IMGFXMaterialLayerParentInterface::AddLayer(UMGFXMaterialLayer* Child, int32 Index)
{
	TArray<TObjectPtr<UMGFXMaterialLayer>>& Children = GetMutableLayers();

	check(Child);
	check(!Children.Contains(Child));

	if (Children.IsValidIndex(Index))
	{
		Children.Insert(Child, Index);
	}
	else
	{
		Children.Add(Child);
	}

	UMGFXMaterialLayer* Parent = Cast<UMGFXMaterialLayer>(this);
	Child->SetParentLayer(Parent);
}

void IMGFXMaterialLayerParentInterface::RemoveLayer(UMGFXMaterialLayer* Child)
{
	TArray<TObjectPtr<UMGFXMaterialLayer>>& Layers = GetMutableLayers();

	check(Layers.Contains(Child));

	Layers.Remove(Child);
	Child->SetParentLayer(nullptr);
}

void IMGFXMaterialLayerParentInterface::ReorderLayer(UMGFXMaterialLayer* Child, int32 NewIndex)
{
	TArray<TObjectPtr<UMGFXMaterialLayer>>& Children = GetMutableLayers();

	check(Children.Contains(Child));

	Children.Remove(Child);
	Children.Insert(Child, NewIndex);
}

bool IMGFXMaterialLayerParentInterface::HasLayer(const UMGFXMaterialLayer* Child) const
{
	return GetLayers().Contains(Child);
}

int32 IMGFXMaterialLayerParentInterface::GetLayerIndex(const UMGFXMaterialLayer* Child) const
{
	return GetLayers().IndexOfByKey(Child);
}

int32 IMGFXMaterialLayerParentInterface::NumLayers() const
{
	return GetLayers().Num();
}

bool IMGFXMaterialLayerParentInterface::HasLayers() const
{
	return NumLayers() > 0;
}

UMGFXMaterialLayer* IMGFXMaterialLayerParentInterface::GetLayer(int32 Index) const
{
	const TArray<TObjectPtr<UMGFXMaterialLayer>>& Children = GetLayers();
	if (Children.IsValidIndex(Index))
	{
		return Children[Index];
	}
	return nullptr;
}


// UMGFXMaterialLayer
// ------------------


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


void UMGFXMaterialLayer::SetParentLayer(UMGFXMaterialLayer* NewParent)
{
	Modify();
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
			Child->SetParentLayer(this);
		}
	}
#endif
}

void UMGFXMaterialLayer::GetAllLayers(TArray<TObjectPtr<UMGFXMaterialLayer>>& OutLayers) const
{
	for (TObjectPtr<UMGFXMaterialLayer> Layer : Children)
	{
		OutLayers.Add(Layer);
		Layer->GetAllLayers(OutLayers);
	}
}
