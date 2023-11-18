// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FMGFXMaterialEditor;
class SMGFXMaterialEditorLayerTreeView;
class UMGFXMaterialLayer;


/**
 * Widget containing a tree view for editing all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayers : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, UMGFXMaterialLayer* /*TreeItem*/);

public:
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorLayers)
		{
		}

		SLATE_ARGUMENT(TWeakPtr<FMGFXMaterialEditor>, MGFXMaterialEditor)

		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditor;

	/** Add a new layer above the selected layers. */
	void AddNewLayerAboveSelection();

	void AddNewLayer(UMGFXMaterialLayer* Parent, int32 Index = 0);

	void AddLayer(UMGFXMaterialLayer* NewLayer, UMGFXMaterialLayer* Parent, int32 Index);

	void ReparentLayer(UMGFXMaterialLayer* Layer, UMGFXMaterialLayer* NewParent, int32 Index);

	void BeginRename();

	bool CanRename();

protected:
	TSharedPtr<SMGFXMaterialEditorLayerTreeView> TreeView;

	FOnSelectionChanged OnSelectionChanged;

	TArray<TObjectPtr<UMGFXMaterialLayer>> TreeRootItems;

	/** Commands specific to the layers view. */
	TSharedPtr<FUICommandList> CommandList;

	/** Flag used to ignore selection changes originating from the tree view. */
	bool bIsUpdatingSelection = false;

	void OnTreeSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnTreeContextMenuOpening();
	void OnSetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> InItem, bool bShouldBeExpanded);

	/** Called when the selection has changed and the tree view should update to match. */
	void OnEditorLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers);

	FReply OnNewLayerButtonClicked();

	// called when a layer is added, removed, or reparented
	void OnLayersChanged();
};
