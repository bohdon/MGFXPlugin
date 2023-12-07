// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorLayerTreeView.h"

#include "MGFXEditorStyle.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "MGFXMaterialEditorUtils.h"
#include "MGFXMaterialLayer.h"
#include "SlateOptMacros.h"
#include "SMGFXMaterialEditorLayers.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// FMGFXMaterialLayerDragDropOp
// ----------------------------

FMGFXMaterialLayerDragDropOp::~FMGFXMaterialLayerDragDropOp()
{
	delete Transaction;
}

void FMGFXMaterialLayerDragDropOp::OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	if (!bDropWasHandled)
	{
		Transaction->Cancel();
	}
}

TSharedRef<FMGFXMaterialLayerDragDropOp> FMGFXMaterialLayerDragDropOp::New(const TArray<TObjectPtr<UMGFXMaterialLayer>>& InLayers)
{
	check(!InLayers.IsEmpty());

	TSharedRef<FMGFXMaterialLayerDragDropOp> Operation = MakeShareable(new FMGFXMaterialLayerDragDropOp());

	// set text and the transaction name based on single or multiple layers
	if (InLayers.Num() == 1)
	{
		Operation->CurrentHoverText = Operation->DefaultHoverText = FText::FromString(InLayers[0]->Name);
		Operation->CurrentIconBrush = FAppStyle::GetBrush(TEXT("Layer.Icon16x"));
		Operation->Transaction = new FScopedTransaction(LOCTEXT("MoveLayer", "Move Layer"));
	}
	else
	{
		Operation->CurrentHoverText = Operation->DefaultHoverText = LOCTEXT("DragMultipleLayers", "Multiple Layers");
		Operation->CurrentIconBrush = FAppStyle::GetBrush(TEXT("LevelEditor.Tabs.Layers"));
		Operation->Transaction = new FScopedTransaction(LOCTEXT("MoveLayers", "Move Layers"));
	}
	Operation->SetupDefaults();

	// add entry for each layer
	for (const auto& Layer : InLayers)
	{
		FItem DraggedItem(Layer, Layer->GetParentLayer());

		DraggedItem.Layer->Modify();
		if (DraggedItem.ParentLayer)
		{
			DraggedItem.ParentLayer->Modify();
		}

		Operation->DraggedItems.Add(DraggedItem);
	}

	Operation->Construct();

	return Operation;
}


// SMGFXMaterialLayerRow
// ---------------------

void SMGFXMaterialLayerRow::Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayerTreeView>& InOwningTreeView)
{
	Item = InArgs._Item;
	OnLayersDroppedEvent = InArgs._OnLayersDropped;

	STableRow::Construct(
		STableRow::FArguments()
		.OnCanAcceptDrop(this, &SMGFXMaterialLayerRow::HandleCanAcceptDrop)
		.OnAcceptDrop(this, &SMGFXMaterialLayerRow::HandleAcceptDrop)
		.OnDragDetected(this, &SMGFXMaterialLayerRow::HandleDragDetected)
		.OnDragEnter(this, &SMGFXMaterialLayerRow::HandleDragEnter)
		.OnDragLeave(this, &SMGFXMaterialLayerRow::HandleDragLeave)
		.Style(&FMGFXEditorStyle::Get().GetWidgetStyle<FTableRowStyle>("MGFXMaterialEditor.Layers.TableViewRow"))
		.Padding(4.f)
		.Content()
		[
			SNew(SBorder)
			.Padding(6.f)
			.BorderImage(this, &SMGFXMaterialLayerRow::GetBorder)
			.Content()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				  .Padding(0.f, 0.f, 6.f, 0.f)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Center)
				  .AutoWidth()
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Image(FAppStyle::Get().GetBrush("Icons.DragHandle"))
				]

				+ SHorizontalBox::Slot()
				  .Padding(0.f, 0.f, 6.f, 0.f)
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				  .AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush(TEXT("Layer.Icon16x")))
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SAssignNew(EditableNameText, SInlineEditableTextBlock)
					.Text_Lambda([this]() { return FText::FromString(Item->Name); })
					.OnVerifyTextChanged(this, &SMGFXMaterialLayerRow::OnVerifyNameTextChanged)
					.OnTextCommitted(this, &SMGFXMaterialLayerRow::OnNameTextCommited)
					.IsSelected(this, &SMGFXMaterialLayerRow::IsSelectedExclusively)
				]
			]
		],
		InOwningTreeView
	);

	// an inner-content border is used to represent selection instead
	SetBorderImage(FAppStyle::GetBrush("NoBorder"));
}

void SMGFXMaterialLayerRow::ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent)
{
	// copied from STableRow::ConstructChildren, but overridden purely to increase indent

	this->Content = InContent;
	InnerContentSlot = nullptr;

	SHorizontalBox::FSlot* InnerContentSlotNativePtr = nullptr;

	// Rows in a TreeView need an expander button and some indentation
	this->ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .HAlign(HAlign_Right)
		  .VAlign(VAlign_Fill)
		[
			SAssignNew(ExpanderArrowWidget, SExpanderArrow, SharedThis(this))
			.StyleSet(ExpanderStyleSet)
			.IndentAmount(20)
			.ShouldDrawWires(false)
		]

		+ SHorizontalBox::Slot()
		  .FillWidth(1)
		  .Expose(InnerContentSlotNativePtr)
		  .Padding(InPadding)
		[
			InContent
		]
	];

	InnerContentSlot = InnerContentSlotNativePtr;
}

const FSlateBrush* SMGFXMaterialLayerRow::GetBorder() const
{
	return STableRow::GetBorder();
}

int32 SMGFXMaterialLayerRow::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                     FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// don't draw focus border

	LayerId = SBorder::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (ItemDropZone.IsSet())
	{
		if (PaintDropIndicatorEvent.IsBound())
		{
			LayerId = PaintDropIndicatorEvent.Execute(ItemDropZone.GetValue(), Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
			                                          bParentEnabled);
		}
		else
		{
			OnPaintDropIndicator(ItemDropZone.GetValue(), Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		}
	}

	return LayerId;
}

TOptional<bool> SMGFXMaterialLayerRow::OnQueryShowFocus(const EFocusCause InFocusCause) const
{
	return TOptional<bool>(false);
}

void SMGFXMaterialLayerRow::BeginRename()
{
	TSharedPtr<SInlineEditableTextBlock> TextBlock = EditableNameText.Pin();
	if (TextBlock.IsValid())
	{
		TextBlock->EnterEditingMode();
	}
}

bool SMGFXMaterialLayerRow::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	if (InText.IsEmpty())
	{
		OutErrorMessage = LOCTEXT("LayerNameEmpty", "Layer names cannot be empty");
		return false;
	}

	const FString NewName = InText.ToString().TrimStartAndEnd();
	if (NewName == Item->Name)
	{
		return true;
	}

	if (!NewName.Equals(FMGFXMaterialEditorUtils::MakeUniqueLayerName(NewName, Item->GetTypedOuter<UMGFXMaterial>())))
	{
		OutErrorMessage = LOCTEXT("LayerNameNotUnique", "Layer names must be unique");
		return false;
	}

	return true;
}

void SMGFXMaterialLayerRow::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		const FString NewName = InText.ToString().TrimStartAndEnd();

		if (!Item->Name.Equals(NewName))
		{
			FScopedTransaction Transaction(LOCTEXT("RenameLayer", "Rename Layer"));
			Item->Modify();
			Item->Name = NewName;
		}
	}
}

FReply SMGFXMaterialLayerRow::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// determine which item or items are being dragged
	TArray<TObjectPtr<UMGFXMaterialLayer>> DraggedItems;

	// drag all selected items
	const TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedItems = OwnerTablePtr.Pin()->GetSelectedItems();
	if (SelectedItems.Num() > 1)
	{
		DraggedItems = SelectedItems;
	}

	// if nothing selected, drag at least this item
	if (DraggedItems.IsEmpty())
	{
		if (Item.IsValid())
		{
			DraggedItems.Add(Item.Get());
		}
	}

	if (!DraggedItems.IsEmpty())
	{
		return FReply::Handled().BeginDragDrop(FMGFXMaterialLayerDragDropOp::New(DraggedItems));
	}

	return FReply::Unhandled();
}

void SMGFXMaterialLayerRow::HandleDragEnter(const FDragDropEvent& DragDropEvent)
{
}

void SMGFXMaterialLayerRow::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	// reset to default hover icon and text
	if (const TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>())
	{
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}
}

TOptional<EItemDropZone> SMGFXMaterialLayerRow::HandleCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone,
                                                                    TObjectPtr<UMGFXMaterialLayer> TargetItem)
{
	// reset to default hover icon and text before evaluating each time
	if (const TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>())
	{
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}

	if (!TargetItem)
	{
		return TOptional<EItemDropZone>();
	}

	// handle layer dragging
	if (TSharedPtr<FMGFXMaterialLayerDragDropOp> LayerDragDropOp = DragDropEvent.GetOperationAs<FMGFXMaterialLayerDragDropOp>())
	{
		// iterating the entire items list multiple times so that we can present the most relevant info first,
		// instead of whichever layer first triggers an issue

		// check for drag onto self
		for (const FMGFXMaterialLayerDragDropOp::FItem& DraggedItem : LayerDragDropOp->DraggedItems)
		{
			if (DraggedItem.Layer == TargetItem)
			{
				// can't drag a layer onto itself, but not important enough to warn about
				return TOptional<EItemDropZone>();
			}
		}

		// check for circular references
		for (const FMGFXMaterialLayerDragDropOp::FItem& DraggedItem : LayerDragDropOp->DraggedItems)
		{
			if (DraggedItem.Layer->IsParentLayer(TargetItem))
			{
				// target item is a child of the dragged layer, can't parent to child of self
				LayerDragDropOp->CurrentIconBrush = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				LayerDragDropOp->CurrentHoverText = LOCTEXT("CantMakeLayerChildOfChildren", "Can't make layer a child of its children.");
				return TOptional<EItemDropZone>();
			}
		}

		// TODO
		// check that the drag is within the same material

		return DropZone;
	}

	return TOptional<EItemDropZone>();
}

FReply SMGFXMaterialLayerRow::HandleAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TObjectPtr<UMGFXMaterialLayer> TargetItem)
{
	if (TSharedPtr<FMGFXMaterialLayerDragDropOp> LayerDragDropOp = DragDropEvent.GetOperationAs<FMGFXMaterialLayerDragDropOp>())
	{
		// convert to avoid ambiguous conditionals
		UMGFXMaterialLayer* TargetLayer = TargetItem;

		// determine the new parent, either the target item, or the target item's parent (which may be the material itself)
		IMGFXMaterialLayerParentInterface* NewContainer = DropZone == EItemDropZone::OntoItem ? TargetLayer : TargetItem->GetParentContainer();

		UObject* ParentObject = CastChecked<UObject>(NewContainer);
		ParentObject->Modify();

		// determine the new index
		int32 NewIndex;
		switch (DropZone)
		{
		default:
		case EItemDropZone::OntoItem:
			NewIndex = 0;
			break;
		case EItemDropZone::AboveItem:
			NewIndex = NewContainer->GetLayerIndex(TargetLayer);
			break;
		case EItemDropZone::BelowItem:
			NewIndex = NewContainer->GetLayerIndex(TargetLayer) + 1;
			break;
		}

		TArray<TObjectPtr<UMGFXMaterialLayer>> DroppedLayers;
		for (const FMGFXMaterialLayerDragDropOp::FItem& DraggedItem : LayerDragDropOp->DraggedItems)
		{
			DraggedItem.Layer->Modify();

			if (NewContainer->HasLayer(DraggedItem.Layer))
			{
				// just reordering an existing child
				NewContainer->ReorderLayer(DraggedItem.Layer, NewIndex);
			}
			else
			{
				// remove from previous container
				if (IMGFXMaterialLayerParentInterface* OldContainer = DraggedItem.Layer->GetParentContainer())
				{
					UObject* OldContainerObject = CastChecked<UObject>(OldContainer);
					OldContainerObject->Modify();

					OldContainer->RemoveLayer(DraggedItem.Layer);
				}

				// add to new one
				NewContainer->AddLayer(DraggedItem.Layer, NewIndex);
			}
			DroppedLayers.Add(DraggedItem.Layer);
		}

		// broadcast event, sending new parent layer (which may be null if parenting to root)
		UMGFXMaterialLayer* NewParentLayer = Cast<UMGFXMaterialLayer>(NewContainer);
		OnLayersDroppedEvent.ExecuteIfBound(NewParentLayer, DroppedLayers);

		return FReply::Handled();
	}
	return FReply::Unhandled();
}


// SMGFXMaterialEditorLayerTreeView
// --------------------------------

void SMGFXMaterialEditorLayerTreeView::Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayers>& InOwningLayers)
{
	STreeView::Construct(
		STreeView::FArguments(InArgs)
		.OnGenerateRow(this, &SMGFXMaterialEditorLayerTreeView::MakeTableRowWidget)
		.OnGetChildren(this, &SMGFXMaterialEditorLayerTreeView::HandleGetChildrenForTree)
	);
}

TSharedRef<ITableRow> SMGFXMaterialEditorLayerTreeView::MakeTableRowWidget(TObjectPtr<UMGFXMaterialLayer> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SMGFXMaterialLayerRow> RowWidget = SNew(SMGFXMaterialLayerRow, SharedThis(this))
		.Item(InItem)
		.OnLayersDropped(this, &SMGFXMaterialEditorLayerTreeView::OnLayersDropped);

	return RowWidget;
}

void SMGFXMaterialEditorLayerTreeView::HandleGetChildrenForTree(TObjectPtr<UMGFXMaterialLayer> InItem, TArray<TObjectPtr<UMGFXMaterialLayer>>& OutChildren)
{
	OutChildren = InItem->GetLayers();
}

void SMGFXMaterialEditorLayerTreeView::SetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> TreeItem, bool bShouldBeExpanded)
{
	SetItemExpansion(TreeItem, bShouldBeExpanded);

	for (int32 Idx = 0; Idx < TreeItem->NumLayers(); ++Idx)
	{
		SetExpansionRecursive(TreeItem->GetLayer(Idx), bShouldBeExpanded);
	}
}

void SMGFXMaterialEditorLayerTreeView::OnLayersDropped(TObjectPtr<UMGFXMaterialLayer> NewParentLayer,
                                                       const TArray<TObjectPtr<UMGFXMaterialLayer>>& DroppedLayers)
{
	// propagate the event upwards
	OnLayersDroppedEvent.ExecuteIfBound(NewParentLayer, DroppedLayers);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
