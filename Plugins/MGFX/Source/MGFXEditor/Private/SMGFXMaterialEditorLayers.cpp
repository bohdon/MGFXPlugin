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

	MGFXMaterial = MGFXMaterialEditor.Pin()->GetMGFXMaterial();

	MGFXMaterialEditor.Pin()->OnLayersChangedEvent.AddRaw(this, &SMGFXMaterialEditorLayers::OnLayersChanged);
	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddRaw(this, &SMGFXMaterialEditorLayers::OnEditorLayerSelectionChanged);

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::BeginRename),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorLayers::CanRename));

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
			.TreeItemsSource(&MGFXMaterial->RootLayers)
			.OnSelectionChanged(this, &SMGFXMaterialEditorLayers::OnTreeSelectionChanged)
			.OnContextMenuOpening(this, &SMGFXMaterialEditorLayers::OnTreeContextMenuOpening)
			.OnSetExpansionRecursive(this, &SMGFXMaterialEditorLayers::OnSetExpansionRecursive)
		]
	];

	// start with all layers expanded
	for (const TObjectPtr<UMGFXMaterialLayer> RootItem : MGFXMaterial->RootLayers)
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
			// even if there's no parent, we can add as a root layer
			UMGFXMaterialLayer* ParentLayer = Layer->GetParentLayer();

			UObject* ParentObject = ParentLayer ? Cast<UObject>(ParentLayer) : MGFXMaterial.Get();
			const IMGFXMaterialLayerParentInterface* Container = CastChecked<IMGFXMaterialLayerParentInterface>(ParentObject);
			const int32 LayerIndex = Container->GetLayerIndex(Layer);

			// add layers at the same index, i.e. right on top
			AddNewLayer(ParentLayer, LayerIndex);
		}
	}
	else
	{
		// add as root layer
		AddNewLayer(nullptr, 0);
	}
}

void SMGFXMaterialEditorLayers::AddNewLayer(UMGFXMaterialLayer* Parent, int32 Index)
{
	// create layer
	UMGFXMaterialLayer* NewLayer = NewObject<UMGFXMaterialLayer>(MGFXMaterial.Get(), NAME_None, RF_Public | RF_Transactional);
	NewLayer->Name = FMGFXMaterialEditor::MakeUniqueLayerName(NewLayer->Name, MGFXMaterial.Get());

	// then add it to the given parent
	AddLayer(NewLayer, Parent, Index);

	TreeView->SetSelection(NewLayer);
}

void SMGFXMaterialEditorLayers::AddLayer(UMGFXMaterialLayer* NewLayer, UMGFXMaterialLayer* Parent, int32 Index)
{
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

void SMGFXMaterialEditorLayers::OnTreeSelectionChanged(TObjectPtr<UMGFXMaterialLayer> TreeItem, ESelectInfo::Type SelectInfo)
{
	// ignore incoming selection change events while broadcasting
	bIsUpdatingSelection = true;

	OnSelectionChanged.ExecuteIfBound(TreeView->GetSelectedItems());

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
