// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorLayerTreeView.h"

#include "MGFXEditorStyle.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "MGFXMaterialLayer.h"
#include "SlateOptMacros.h"
#include "SMGFXMaterialEditorLayers.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// SMGFXMaterialLayerRow
// ---------------------

void SMGFXMaterialLayerRow::Construct(const FArguments& InArgs, const TSharedRef<SMGFXMaterialEditorLayerTreeView>& InOwningTreeView)
{
	OwningTreeView = InOwningTreeView;
	Item = InArgs._Item;

	STableRow::Construct(
		STableRow::FArguments()
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
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(Item->Name); })
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
	TSharedRef<SMGFXMaterialLayerRow> RowWidget = SNew(SMGFXMaterialLayerRow, SharedThis(this)).Item(InItem);

	return RowWidget;
}

void SMGFXMaterialEditorLayerTreeView::HandleGetChildrenForTree(TObjectPtr<UMGFXMaterialLayer> InItem, TArray<TObjectPtr<UMGFXMaterialLayer>>& OutChildren)
{
	OutChildren = InItem->GetChildren();
}

void SMGFXMaterialEditorLayerTreeView::SetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> TreeItem, bool bShouldBeExpanded)
{
	SetItemExpansion(TreeItem, bShouldBeExpanded);

	for (int32 Idx = 0; Idx < TreeItem->NumChildren(); ++Idx)
	{
		SetExpansionRecursive(TreeItem->GetChild(Idx), bShouldBeExpanded);
	}
}
