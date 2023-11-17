// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FMGFXMaterialEditor;
class SMGFXMaterialEditorLayerTreeView;
class SMGFXMaterialEditorLayers;
class UMGFXMaterial;
class UMGFXMaterialLayer;


class SMGFXMaterialLayerRow : public STableRow<TObjectPtr<UMGFXMaterialLayer>>
{
public:
	SLATE_BEGIN_ARGS(SMGFXMaterialLayerRow)
		{
		}

		/** The list item for this row */
		SLATE_ARGUMENT(TObjectPtr<UMGFXMaterialLayer>, Item)

	SLATE_END_ARGS()

	/** Construct function for this widget */
	void Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayerTreeView>& InOwningTreeView);
	virtual void ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent) override;

	virtual const FSlateBrush* GetBorder() const override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual TOptional<bool> OnQueryShowFocus(const EFocusCause InFocusCause) const override;

protected:
	/** The item associated with this row of data */
	TWeakObjectPtr<UMGFXMaterialLayer> Item;

	/** Weak pointer to the owning tree view */
	TWeakPtr<SMGFXMaterialEditorLayerTreeView> OwningTreeView;

	FSlateBrush LayerIconBrush;
};


/**
 * Tree view displaying all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayerTreeView : public STreeView<TObjectPtr<UMGFXMaterialLayer>>
{
public:
	void Construct(const FArguments& InArgs, TSharedRef<SMGFXMaterialEditorLayers> InOwningLayers);

	TSharedRef<ITableRow> MakeTableRowWidget(TObjectPtr<UMGFXMaterialLayer> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TObjectPtr<UMGFXMaterialLayer> InItem, TArray<TObjectPtr<UMGFXMaterialLayer>>& OutChildren);

	void SetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> InItem, bool bTowardsParent, bool bShouldBeExpanded);
};


/**
 * Widget containing a tree view for editing all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayers : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TObjectPtr<UMGFXMaterialLayer> /*TreeItem*/);

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
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditorPtr;

protected:
	TSharedPtr<SMGFXMaterialEditorLayerTreeView> TreeView;

	FOnSelectionChanged OnSelectionChanged;

	TArray<TObjectPtr<UMGFXMaterialLayer>> TreeRootItems;

	void HandleSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo);
};
