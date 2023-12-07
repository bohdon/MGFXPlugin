// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialLayer.h"

#include "MGFXMaterial.h"
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

	const int32 OldIndex = Children.IndexOfByKey(Child);
	if (OldIndex == NewIndex)
	{
		// nothing to do
		return;
	}

	Children.Remove(Child);

	// insert, but at Index - 1 if we removed an item that was before the new index
	Children.Insert(Child, OldIndex < NewIndex ? NewIndex - 1 : NewIndex);
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


IMGFXMaterialLayerParentInterface* UMGFXMaterialLayer::GetParentContainer() const
{
	if (UMGFXMaterialLayer* ParentLayer = GetParentLayer())
	{
		return ParentLayer;
	}
	else if (UMGFXMaterial* Material = GetTypedOuter<UMGFXMaterial>())
	{
		return Material;
	}
	return nullptr;
}

void UMGFXMaterialLayer::SetParentLayer(UMGFXMaterialLayer* NewParent)
{
	Modify();
	Parent = NewParent;
}

bool UMGFXMaterialLayer::IsParentLayer(const UMGFXMaterialLayer* Layer) const
{
	check(Layer);

	const UMGFXMaterialLayer* ParentLayer = Layer->GetParentLayer();
	while (ParentLayer)
	{
		if (ParentLayer == this)
		{
			return true;
		}
		ParentLayer = ParentLayer->GetParentLayer();
	}

	return false;
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

void UMGFXMaterialLayer::GetAllLayers(TArray<UMGFXMaterialLayer*>& OutLayers) const
{
	for (UMGFXMaterialLayer* Layer : Children)
	{
		OutLayers.Add(Layer);
		Layer->GetAllLayers(OutLayers);
	}
}

#if WITH_EDITOR
void UMGFXMaterialLayer::PreEditChange(FProperty* PropertyAboutToChange)
{
	UObject::PreEditChange(PropertyAboutToChange);

	if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, Shape))
	{
		LastKnownShape = Shape;
	}
}

void UMGFXMaterialLayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
	{
		if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, Shape) &&
			PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, Shape))
		{
			if (Shape && LastKnownShape.IsSet() && Shape != LastKnownShape.GetValue())
			{
				// there is an issue that causes a crash when importing layers via text (copy/paste in editor) and
				// any subobject has a nested Instanced object property with a default object assigned in the constructor.
				// so instead the default visual is added later, in this case whenever the user changes the Shape property of a layer.
				Shape->AddDefaultVisual();
			}
		}
	}

	LastKnownShape.Reset();
}
#endif
