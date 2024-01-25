// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorLayers.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "MGFXMaterialEditorUtils.h"
#include "MGFXMaterialLayer.h"
#include "ScopedTransaction.h"
#include "SlateOptMacros.h"
#include "SMGFXMaterialEditorLayerTreeView.h"
#include "Framework/Commands/GenericCommands.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Shapes/MGFXMaterialShape_Circle.h"
#include "Shapes/MGFXMaterialShape_Rect.h"
#include "Shapes/MGFXMaterialShape_Triangle.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// SMGFXMaterialEditorLayers
// -------------------------

void SMGFXMaterialEditorLayers::Construct(const FArguments& InArgs)
{
	check(InArgs._MGFXMaterialEditor.IsValid());

	MGFXMaterialEditor = InArgs._MGFXMaterialEditor;
	OnSelectionChangedEvent = InArgs._OnSelectionChanged;
	OnLayersChangedEvent = InArgs._OnLayersChanged;

	MGFXMaterial = MGFXMaterialEditor.Pin()->GetMGFXMaterial();

	MGFXMaterialEditor.Pin()->OnLayersChangedEvent.AddSP(this, &SMGFXMaterialEditorLayers::OnEditorLayersChanged);
	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddSP(this, &SMGFXMaterialEditorLayers::OnEditorLayerSelectionChanged);

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::BeginRename),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::CanRename));

	// TODO: dynamically build palette
	const TSubclassOf<UMGFXMaterialShape> CircleShapeClass(UMGFXMaterialShape_Circle::StaticClass());
	const TSubclassOf<UMGFXMaterialShape> RectShapeClass(UMGFXMaterialShape_Rect::StaticClass());
	const TSubclassOf<UMGFXMaterialShape> TriangleShapeClass(UMGFXMaterialShape_Triangle::StaticClass());

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.Padding(6.f)
			[
				SNew(SButton)
				.OnClicked(this, &SMGFXMaterialEditorLayers::OnNewLayerButtonClicked, CircleShapeClass)
				.Content()
				[
					SNew(STextBlock)
					.Text(INVTEXT("Circle"))
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(6.f)
			[
				SNew(SButton)
				.OnClicked(this, &SMGFXMaterialEditorLayers::OnNewLayerButtonClicked, RectShapeClass)
				.Content()
				[
					SNew(STextBlock)
					.Text(INVTEXT("Rect"))
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(6.f)
			[
				SNew(SButton)
					.OnClicked(this, &SMGFXMaterialEditorLayers::OnNewLayerButtonClicked, TriangleShapeClass)
					.Content()
				[
					SNew(STextBlock)
					.Text(INVTEXT("Triangle"))
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(6.f)
			[
				SNew(SButton)
				.OnClicked(this, &SMGFXMaterialEditorLayers::OnNewLayerButtonClicked, TSubclassOf<UMGFXMaterialShape>())
				.Content()
				[
					SNew(STextBlock)
					.Text(INVTEXT("+"))
				]
			]
		]

		+ SVerticalBox::Slot()
		[
			SAssignNew(TreeView, SMGFXMaterialEditorLayerTreeView, SharedThis(this))
			.SelectionMode(ESelectionMode::Multi)
			.TreeItemsSource(&MGFXMaterial->RootLayers)
			.OnSelectionChanged(this, &SMGFXMaterialEditorLayers::OnTreeSelectionChanged)
			.OnContextMenuOpening(this, &SMGFXMaterialEditorLayers::OnTreeContextMenuOpening)
			.OnSetExpansionRecursive(this, &SMGFXMaterialEditorLayers::OnSetExpansionRecursive)
		]
	];

	TreeView->OnLayersDroppedEvent.BindRaw(this, &SMGFXMaterialEditorLayers::OnLayersDropped);

	// start with all layers expanded
	for (const TObjectPtr<UMGFXMaterialLayer> RootItem : MGFXMaterial->RootLayers)
	{
		TreeView->SetExpansionRecursive(RootItem, true);
	}
}


UMGFXMaterialLayer* SMGFXMaterialEditorLayers::CreateNewLayer()
{
	// create layer
	UMGFXMaterialLayer* NewLayer = NewObject<UMGFXMaterialLayer>(MGFXMaterial.Get(), NAME_None, RF_Public | RF_Transactional);
	NewLayer->Name = FMGFXMaterialEditorUtils::MakeUniqueLayerName(NewLayer->Name, MGFXMaterial.Get());
	return NewLayer;
}

void SMGFXMaterialEditorLayers::AddLayerAboveSelection(UMGFXMaterialLayer* NewLayer)
{
	TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	if (SelectedLayers.IsValidIndex(0))
	{
		const UMGFXMaterialLayer* SelectedLayer = SelectedLayers[0];

		// even if there's no parent, we can add as a root layer
		UMGFXMaterialLayer* ParentLayer = SelectedLayer->GetParentLayer();

		UObject* ParentObject = ParentLayer ? Cast<UObject>(ParentLayer) : MGFXMaterial.Get();
		const IMGFXMaterialLayerParentInterface* Container = CastChecked<IMGFXMaterialLayerParentInterface>(ParentObject);
		const int32 LayerIndex = Container->GetLayerIndex(SelectedLayer);

		// add layers at the same index, i.e. right on top
		AddLayer(NewLayer, ParentLayer, LayerIndex);
	}
	else
	{
		// add as root layer
		AddLayer(NewLayer, nullptr, 0);
	}
}

void SMGFXMaterialEditorLayers::AddLayer(UMGFXMaterialLayer* NewLayer, UMGFXMaterialLayer* Parent, int32 Index)
{
	FScopedTransaction Transaction(LOCTEXT("AddLayer", "Add Layer"));

	UObject* ParentObject = Parent ? Cast<UObject>(Parent) : MGFXMaterial.Get();
	IMGFXMaterialLayerParentInterface* Container = CastChecked<IMGFXMaterialLayerParentInterface>(ParentObject);
	ParentObject->Modify();

	Container->AddLayer(NewLayer, Index);

	if (Parent)
	{
		// expand parent
		TreeView->SetItemExpansion(Parent, true);
	}

	TreeView->RequestTreeRefresh();
}

void SMGFXMaterialEditorLayers::ReparentLayer(UMGFXMaterialLayer* Layer, UMGFXMaterialLayer* NewParent, int32 Index)
{
	FScopedTransaction Transaction(LOCTEXT("ReparentLayer", "Reparent Layer"));

	UObject* ParentObject = NewParent ? Cast<UObject>(NewParent) : MGFXMaterial.Get();
	IMGFXMaterialLayerParentInterface* Container = CastChecked<IMGFXMaterialLayerParentInterface>(ParentObject);
	ParentObject->Modify();

	if (Container->HasLayer(Layer))
	{
		// just reordering an existing child
		Container->ReorderLayer(Layer, Index);
	}
	else
	{
		Container->AddLayer(Layer, Index);
	}

	if (NewParent)
	{
		// expand parent
		TreeView->SetItemExpansion(NewParent, true);
	}

	TreeView->RequestTreeRefresh();

	OnLayersChangedEvent.ExecuteIfBound();
}


void SMGFXMaterialEditorLayers::BeginRename()
{
	TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	if (SelectedLayers.Num() == 1)
	{
		const TSharedPtr<ITableRow> LayerWidget = TreeView->WidgetFromItem(SelectedLayers[0]);
		TSharedPtr<SMGFXMaterialLayerRow> RowWidget = StaticCastSharedPtr<SMGFXMaterialLayerRow>(LayerWidget);
		if (RowWidget.IsValid())
		{
			RowWidget->BeginRename();
		}
	}
}

bool SMGFXMaterialEditorLayers::CanRename()
{
	const TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	return SelectedLayers.Num() == 1;
}

FReply SMGFXMaterialEditorLayers::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SMGFXMaterialEditorLayers::OnLayersDropped(TObjectPtr<UMGFXMaterialLayer> NewParentLayer, const TArray<TObjectPtr<UMGFXMaterialLayer>>& DroppedLayers)
{
	TreeView->RequestTreeRefresh();

	OnLayersChangedEvent.ExecuteIfBound();
}

void SMGFXMaterialEditorLayers::OnTreeSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo)
{
	// ignore incoming selection change events while broadcasting
	bIsUpdatingSelection = true;

	OnSelectionChangedEvent.ExecuteIfBound(TreeView->GetSelectedItems());

	bIsUpdatingSelection = false;
}

TSharedPtr<SWidget> SMGFXMaterialEditorLayers::OnTreeContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, CommandList);

	MenuBuilder.BeginSection("Edit", LOCTEXT("Edit", "Edit"));
	{
		MenuBuilder.PushCommandList(MGFXMaterialEditor.Pin()->GetToolkitCommands());
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.PopCommandList();

		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SMGFXMaterialEditorLayers::OnSetExpansionRecursive(TObjectPtr<UMGFXMaterialLayer> InItem, bool bShouldBeExpanded)
{
	TreeView->SetExpansionRecursive(InItem, bShouldBeExpanded);
}

void SMGFXMaterialEditorLayers::OnEditorLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers)
{
	if (!bIsUpdatingSelection)
	{
		TreeView->ClearSelection();
		TreeView->SetItemSelection(SelectedLayers, true, ESelectInfo::Direct);
	}
}

FReply SMGFXMaterialEditorLayers::OnNewLayerButtonClicked(TSubclassOf<UMGFXMaterialShape> ShapeClass)
{
	FScopedTransaction Transaction(LOCTEXT("AddLayer", "Add Layer"));

	UMGFXMaterialLayer* NewLayer = CreateNewLayer();
	if (ShapeClass)
	{
		NewLayer->Shape = NewObject<UMGFXMaterialShape>(NewLayer, ShapeClass, NAME_None, RF_Public | RF_Transactional);
		NewLayer->Shape->AddDefaultVisual();
		NewLayer->Name = FMGFXMaterialEditorUtils::MakeUniqueLayerName(NewLayer->Shape->GetShapeName(), MGFXMaterial.Get());
	}

	AddLayerAboveSelection(NewLayer);

	TreeView->SetSelection(NewLayer);

	OnLayersChangedEvent.ExecuteIfBound();

	return FReply::Handled();
}

void SMGFXMaterialEditorLayers::OnEditorLayersChanged()
{
	TreeView->RequestTreeRefresh();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
