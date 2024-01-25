// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandList.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Widgets/SCompoundWidget.h"

class FMGFXMaterialEditor;
class SMGFXMaterialEditorLayerTreeView;
class UMGFXMaterial;
class UMGFXMaterialLayer;


/**
 * Widget containing a tree view for editing all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayers : public SCompoundWidget
{
public:
	DECLARE_DELEGATE(FLayersChangedDelegate);

	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TArray<TObjectPtr<UMGFXMaterialLayer>> /*SelectedLayers*/);

public:
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorLayers)
		{
		}

		SLATE_ARGUMENT(TWeakPtr<FMGFXMaterialEditor>, MGFXMaterialEditor)

		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
		SLATE_EVENT(FLayersChangedDelegate, OnLayersChanged)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditor;

	UMGFXMaterialLayer* CreateNewLayer();

	/** Add a new layer above the selected layers. */
	void AddLayerAboveSelection(UMGFXMaterialLayer* NewLayer);

	void AddLayer(UMGFXMaterialLayer* NewLayer, UMGFXMaterialLayer* Parent, int32 Index);

	void ReparentLayer(UMGFXMaterialLayer* Layer, UMGFXMaterialLayer* NewParent, int32 Index);

	void BeginRename();

	bool CanRename();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	/** Weak pointer to the material being edited. */
	TWeakObjectPtr<UMGFXMaterial> MGFXMaterial;

	TSharedPtr<SMGFXMaterialEditorLayerTreeView> TreeView;

	/** Called when the selection is changed caused by the tree view of this widget. */
	FOnSelectionChanged OnSelectionChangedEvent;

	/** Called whenever a layer is added, removed, or reparented by this widget. */
	FLayersChangedDelegate OnLayersChangedEvent;

	/** Commands specific to the layers view. */
	TSharedPtr<FUICommandList> CommandList;

	/** Flag used to ignore selection changes originating from the tree view. */
	bool bIsUpdatingSelection = false;

	/** Flag used to ignore layer change events originating from the editor. */
	bool bIsBroadcastingLayerChange = false;

	void OnTreeSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnTreeContextMenuOpening();
	void OnSetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> InItem, bool bShouldBeExpanded);

	/** Called when one or more layers is dragged and dropped in the tree view. */
	void OnLayersDropped(TObjectPtr<UMGFXMaterialLayer> NewParentLayer, const TArray<TObjectPtr<UMGFXMaterialLayer>>& DroppedLayers);

	// called when a layer is added, removed, or reparented
	void OnEditorLayersChanged();

	/** Called when the selection has changed and the tree view should update to match. */
	void OnEditorLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers);

	FReply OnNewLayerButtonClicked(TSubclassOf<UMGFXMaterialShape> ShapeClass = nullptr);
};
