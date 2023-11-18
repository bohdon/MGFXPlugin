// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorLayers.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "MGFXMaterialLayer.h"
#include "SlateOptMacros.h"
#include "SMGFXMaterialEditorLayerTreeView.h"
#include "Framework/Commands/GenericCommands.h"

#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// SMGFXMaterialEditorLayers
// -------------------------

void SMGFXMaterialEditorLayers::Construct(const FArguments& InArgs)
{
	check(InArgs._MGFXMaterialEditor.IsValid());

	MGFXMaterialEditor = InArgs._MGFXMaterialEditor;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	MGFXMaterialEditor.Pin()->OnLayersChangedEvent.AddRaw(this, &SMGFXMaterialEditorLayers::OnLayersChanged);
	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddRaw(this, &SMGFXMaterialEditorLayers::OnEditorLayerSelectionChanged);

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::BeginRename),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::CanRename));

	const UMGFXMaterial* MGFXMaterial = MGFXMaterialEditor.Pin()->GetOriginalMGFXMaterial();
	TreeRootItems.Add(MGFXMaterial->RootLayer);

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
				.OnClicked(this, &SMGFXMaterialEditorLayers::OnNewLayerButtonClicked)
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
			.TreeItemsSource(&TreeRootItems)
			.OnSelectionChanged(this, &SMGFXMaterialEditorLayers::OnTreeSelectionChanged)
			.OnContextMenuOpening(this, &SMGFXMaterialEditorLayers::OnTreeContextMenuOpening)
			.OnSetExpansionRecursive(this, &SMGFXMaterialEditorLayers::OnSetExpansionRecursive)
		]
	];

	// start with all layers expanded
	for (const TObjectPtr<UMGFXMaterialLayer> RootItem : TreeRootItems)
	{
		TreeView->SetExpansionRecursive(RootItem, true);
	}
}

void SMGFXMaterialEditorLayers::AddNewLayerAboveSelection()
{
	TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	if (!SelectedLayers.IsEmpty())
	{
		for (const UMGFXMaterialLayer* Layer : SelectedLayers)
		{
			const int32 LayerIndex = Layer->GetIndexInParent();
			// even if there's no parent, we can add as a root layer
			UMGFXMaterialLayer* ParentLayer = Layer->GetParent();

			// add layers at the same index, i.e. right on top
			AddNewLayer(ParentLayer, LayerIndex);
		}
	}
	else
	{
		// add to root layer
		AddNewLayer(MGFXMaterialEditor.Pin()->GetOriginalMGFXMaterial()->RootLayer, 0);
	}
}

void SMGFXMaterialEditorLayers::AddNewLayer(UMGFXMaterialLayer* Parent, int32 Index)
{
	UMGFXMaterial* MGFXMaterial = MGFXMaterialEditor.Pin()->GetOriginalMGFXMaterial();

	UMGFXMaterialLayer* NewLayer = NewObject<UMGFXMaterialLayer>(MGFXMaterial, NAME_None, RF_Public | RF_Transactional);
	AddLayer(NewLayer, Parent, Index);
}

void SMGFXMaterialEditorLayers::AddLayer(UMGFXMaterialLayer* NewLayer, UMGFXMaterialLayer* Parent, int32 Index)
{
	// TODO: add to root layers, don't require a parent
	if (!Parent)
	{
		// no parent, add to root layer
		Parent = MGFXMaterialEditor.Pin()->GetOriginalMGFXMaterial()->RootLayer;
	}

	check(!Parent->GetChildren().Contains(NewLayer));

	Parent->AddChild(NewLayer, Index);
	Parent->Modify();

	// expand parent
	TreeView->SetItemExpansion(Parent, true);
	TreeView->RequestTreeRefresh();
}

void SMGFXMaterialEditorLayers::ReparentLayer(UMGFXMaterialLayer* Layer, UMGFXMaterialLayer* NewParent, int32 Index)
{
	// TODO: add to root layers, don't require a parent
	if (!NewParent)
	{
		NewParent = MGFXMaterialEditor.Pin()->GetOriginalMGFXMaterial()->RootLayer;
	}

	if (NewParent->GetChildren().Contains(Layer))
	{
		// just reordering an existing child
		NewParent->ReorderChild(Layer, Index);
	}
	else
	{
		NewParent->AddChild(Layer, Index);
	}
	NewParent->Modify();

	// expand new parent
	TreeView->SetItemExpansion(NewParent, true);
	TreeView->RequestTreeRefresh();
}


void SMGFXMaterialEditorLayers::BeginRename()
{
}

bool SMGFXMaterialEditorLayers::CanRename()
{
	const TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	return SelectedLayers.Num() == 1;
}

void SMGFXMaterialEditorLayers::OnTreeSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo)
{
	// ignore incoming selection change events while broadcasting
	bIsUpdatingSelection = true;

	OnSelectionChanged.ExecuteIfBound(TreeItem);

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

FReply SMGFXMaterialEditorLayers::OnNewLayerButtonClicked()
{
	AddNewLayerAboveSelection();

	return FReply::Handled();
}

void SMGFXMaterialEditorLayers::OnLayersChanged()
{
	TreeView->RequestTreeRefresh();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
