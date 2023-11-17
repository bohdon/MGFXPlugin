// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FMGFXMaterialEditor;
class SMGFXMaterialEditorLayerTreeView;
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
	void Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayerTreeView>& InOwningTreeView, UMGFXMaterial* InMaterial);
	virtual void ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent) override;

	virtual const FSlateBrush* GetBorder() const override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual TOptional<bool> OnQueryShowFocus(const EFocusCause InFocusCause) const override;

protected:
	/** The item associated with this row of data */
	TWeakObjectPtr<UMGFXMaterialLayer> Item;

	/** Weak pointer to the material who's layers are being edited. */
	TWeakObjectPtr<UMGFXMaterial> Material;

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
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorLayerTreeView)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UMGFXMaterial* InMaterial);

	TSharedRef<ITableRow> MakeTableRowWidget(TObjectPtr<UMGFXMaterialLayer> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TObjectPtr<UMGFXMaterialLayer> InItem, TArray<TObjectPtr<UMGFXMaterialLayer>>& OutChildren);

protected:
	/** Weak pointer to the material who's layers are being edited. */
	TWeakObjectPtr<UMGFXMaterial> Material;

	TArray<TObjectPtr<UMGFXMaterialLayer>> RootElements;
};


/**
 * Widget containing a tree view for editing all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayers : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorLayers)
		{
		}

		SLATE_ARGUMENT(TWeakPtr<FMGFXMaterialEditor>, MGFXMaterialEditor)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditorPtr;

protected:
	TSharedPtr<SMGFXMaterialEditorLayerTreeView> TreeView;
};
