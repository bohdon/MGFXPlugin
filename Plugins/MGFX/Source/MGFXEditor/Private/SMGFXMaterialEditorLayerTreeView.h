// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialLayer.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"

class FMGFXMaterialEditor;
class SMGFXMaterialEditorLayerTreeView;
class SMGFXMaterialEditorLayers;
class UMGFXMaterialLayer;


class FMGFXMaterialLayerDragDropOp : public FDecoratedDragDropOp
{
public:
	struct FItem
	{
		/** The widget being dragged and dropped */
		TObjectPtr<UMGFXMaterialLayer> Layer;

		/** The original parent of the layer. */
		TObjectPtr<UMGFXMaterialLayer> ParentLayer;
	};

	DRAG_DROP_OPERATOR_TYPE(FMGFXMaterialLayerDragDropOp, FDecoratedDragDropOp)

	virtual ~FMGFXMaterialLayerDragDropOp();

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override;

	TArray<FItem> DraggedItems;

	/** Undo transaction that lives for the same lifetime as this drag operation. */
	FScopedTransaction* Transaction;

public:
	/** Construct a new drag/drop operation */
	static TSharedRef<FMGFXMaterialLayerDragDropOp> New(const TArray<TObjectPtr<UMGFXMaterialLayer>>& InLayers);
};


/**
 * The widget displayed for each layer in a UMGFXMaterial.
 */
class SMGFXMaterialLayerRow : public STableRow<TObjectPtr<UMGFXMaterialLayer>>
{
public:
	DECLARE_DELEGATE_TwoParams(FLayersDroppedDelegate,
	                           TObjectPtr<UMGFXMaterialLayer> /*NewParentLayer*/,
	                           const TArray<TObjectPtr<UMGFXMaterialLayer>>& /*DroppedLayers*/);

public:
	SLATE_BEGIN_ARGS(SMGFXMaterialLayerRow)
		{
		}

		/** The list item for this row */
		SLATE_ARGUMENT(TObjectPtr<UMGFXMaterialLayer>, Item)
		SLATE_EVENT(FLayersDroppedDelegate, OnLayersDropped)

	SLATE_END_ARGS()

	/** Construct function for this widget */
	void Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayerTreeView>& InOwningTreeView);
	virtual void ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent) override;

	virtual const FSlateBrush* GetBorder() const override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual TOptional<bool> OnQueryShowFocus(const EFocusCause InFocusCause) const override;

	/** Make the name text editable to start renaming this layer. */
	void BeginRename();

	/** Verify that a new name is valid during a renaming. */
	bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

	/** Commit a new layer name. */
	void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

	FReply HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void HandleDragEnter(const FDragDropEvent& DragDropEvent);
	void HandleDragLeave(const FDragDropEvent& DragDropEvent);

	TOptional<EItemDropZone> HandleCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone,
	                                             TObjectPtr<UMGFXMaterialLayer> TargetItem);
	FReply HandleAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TObjectPtr<UMGFXMaterialLayer> TargetItem);

protected:
	/** The item associated with this row of data */
	TWeakObjectPtr<UMGFXMaterialLayer> Item;

	/** Editable text block for the layer name. */
	TWeakPtr<SInlineEditableTextBlock> EditableNameText;

	/** Called when one or more layers is drag and dropped onto a new layer. */
	FLayersDroppedDelegate OnLayersDroppedEvent;

	FSlateBrush LayerIconBrush;
};


/**
 * Tree view displaying all layers in a UMGFXMaterial.
 */
class MGFXEDITOR_API SMGFXMaterialEditorLayerTreeView : public STreeView<TObjectPtr<UMGFXMaterialLayer>>
{
public:
	void Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayers>& InOwningLayers);

	TSharedRef<ITableRow> MakeTableRowWidget(TObjectPtr<UMGFXMaterialLayer> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildrenForTree(TObjectPtr<UMGFXMaterialLayer> InItem, TArray<TObjectPtr<UMGFXMaterialLayer>>& OutChildren);

	void SetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> InItem, bool bShouldBeExpanded);

	/** Called when one or more layers is drag and dropped onto a new layer. */
	SMGFXMaterialLayerRow::FLayersDroppedDelegate OnLayersDroppedEvent;

protected:
	/** Called from table row widgets when one or more layers is drag and dropped onto a new layer. */
	void OnLayersDropped(TObjectPtr<UMGFXMaterialLayer> NewParentLayer, const TArray<TObjectPtr<UMGFXMaterialLayer>>& DroppedLayers);
};
