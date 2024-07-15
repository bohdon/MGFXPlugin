// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "AssetToolsModule.h"
#include "IMaterialEditor.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MGFXMaterialEditorUtils.h"
#include "MGFXMaterialGenerator.h"
#include "MGFXMaterialLayer.h"
#include "ObjectEditorUtils.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SMGFXMaterialEditorCanvas.h"
#include "SMGFXMaterialEditorLayers.h"
#include "Factories/MaterialFactoryNew.h"
#include "Framework/Commands/GenericCommands.h"
#include "HAL/PlatformApplicationMisc.h"
#include "MaterialEditor/PreviewMaterial.h"
#include "MaterialGraph/MaterialGraph.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/TransactionObjectEvent.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Shapes/MGFXMaterialShapeVisual.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"
#include "UObject/PropertyAccessUtil.h"
#include "Widgets/Docking/SDockTab.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


constexpr auto ReadOnlyFlags = PropertyAccessUtil::EditorReadOnlyFlags;
constexpr auto NotifyMode = EPropertyAccessChangeNotifyMode::Default;

const FName FMGFXMaterialEditor::CanvasTabId(TEXT("MGFXMaterialEditorCanvasTab"));
const FName FMGFXMaterialEditor::LayersTabId(TEXT("MGFXMaterialEditorLayersTab"));
const FName FMGFXMaterialEditor::DetailsTabId(TEXT("MGFXMaterialEditorDetailsTab"));


FMGFXMaterialEditor::FMGFXMaterialEditor()
{
}

FMGFXMaterialEditor::~FMGFXMaterialEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FMGFXMaterialEditor::InitMGFXMaterialEditor(const EToolkitMode::Type Mode,
                                                 const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                 UMGFXMaterial* InMGFXMaterial)
{
	check(InMGFXMaterial);

	MGFXMaterial = InMGFXMaterial;
	CreatePreviewMaterials();

	Generator = MakeShared<FMGFXMaterialGenerator>();

	FMGFXMaterialEditorCommands::Register();
	BindCommands();

	GEditor->RegisterForUndo(this);

	// initialize details view
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FMGFXMaterialEditor::IsDetailsPropertyVisible));
	DetailsView->SetIsCustomRowVisibleDelegate(FIsCustomRowVisible::CreateSP(this, &FMGFXMaterialEditor::IsDetailsRowVisible));
	DetailsView->SetObject(MGFXMaterial);

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MGFXMaterialEditor_v0.1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(LayersTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(CanvasTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
			)
		);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FMGFXEditorModule::MGFXMaterialEditorAppIdentifier, StandaloneDefaultLayout,
	                bCreateDefaultStandaloneMenu, bCreateDefaultToolbar,
	                MGFXMaterial);

	RegisterToolbar();
	RegenerateMenusAndToolbars();
	RegeneratePreviewMaterial();
}

FName FMGFXMaterialEditor::GetToolkitFName() const
{
	return FName("MGFXMaterialEditor");
}

FText FMGFXMaterialEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "MGFX Editor");
}

FString FMGFXMaterialEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "MGFXMaterial ").ToString();
}

FLinearColor FMGFXMaterialEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void FMGFXMaterialEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	IMGFXMaterialEditor::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MGFXMaterialEditor", "MGFX Material Editor"));
	const TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	InTabManager->RegisterTabSpawner(CanvasTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Canvas))
	            .SetDisplayName(LOCTEXT("CanvasTabTitle", "Canvas"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(LayersTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Layers))
	            .SetDisplayName(LOCTEXT("LayersTabTitle", "Layers"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Layers"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Details))
	            .SetDisplayName(LOCTEXT("DetailsTabTitle", "Details"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

UMaterial* FMGFXMaterialEditor::GetTargetMaterial() const
{
	check(MGFXMaterial);
	return MGFXMaterial->Material;
}

void FMGFXMaterialEditor::RegeneratePreviewMaterial()
{
	PreviewMaterial->Modify();
	Generator->Generate(MGFXMaterial, PreviewMaterial, true, true);

	// any interactive parameter changes should be cleared now
	PreviewMID->ClearParameterValues();

	// TODO: use events to keep this up to date
	// update canvas artboard
	CanvasWidget->UpdateArtboardSize();
}

void FMGFXMaterialEditor::Apply()
{
	RegeneratePreviewMaterial();
	RegenerateTargetMaterial();
}

void FMGFXMaterialEditor::RegenerateTargetMaterial()
{
	SCOPED_NAMED_EVENT(FMGFXMaterialEditor_RegenerateMaterial, FColor::Green);

	// create a transaction to group changes, but don't register it,
	// undo/redo for MGFXMaterial changes should not be interrupted by material updates
	const FScopedTransaction Transaction(LOCTEXT("RegenerateMaterial", "Apply MGFX Material"), false);

	UMaterial* Material = GetTargetMaterial();
	if (!Material)
	{
		Material = CreateTargetMaterialAsset();

		if (!Material)
		{
			UE_LOG(LogMGFXEditor, Error, TEXT("Failed to create Material for %s"), *GetNameSafe(MGFXMaterial));
			return;
		}
	}

	// if MaterialEditor is open, update its preview material as well
	if (IMaterialEditor* MaterialEditor = FindMaterialEditor(Material))
	{
		UPreviewMaterial* EditorPreviewMaterial = GetMaterialEditorPreviewMaterial(MaterialEditor);
		check(EditorPreviewMaterial);

		// could try copying the material expressions, but generating seems to be faster
		EditorPreviewMaterial->Modify();
		Generator->Generate(MGFXMaterial, EditorPreviewMaterial, true, false);

		EditorPreviewMaterial->MaterialGraph->RebuildGraph();

		// notify the editor, so it can recompile and display stats
		MaterialEditor->NotifyExternalMaterialChange();
	}

	Material->Modify();
	Generator->Generate(MGFXMaterial, Material, true, false);
}

UMaterial* FMGFXMaterialEditor::CreateTargetMaterialAsset()
{
	if (!MGFXMaterial->Material)
	{
		const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		const FString MGFXMaterialPath = MGFXMaterial->GetPackage()->GetPathName();
		const FString PackagePath = FPackageName::GetLongPackagePath(MGFXMaterialPath);
		const FString AssetName = TEXT("M_") + FPackageName::GetShortName(MGFXMaterialPath);

		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		UObject* NewMaterial = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UMaterial::StaticClass(), MaterialFactory);

		if (NewMaterial)
		{
			MGFXMaterial->Material = Cast<UMaterial>(NewMaterial);
			OnMaterialAssetChanged();
		}
	}

	return MGFXMaterial->Material;
}

FVector2D FMGFXMaterialEditor::GetCanvasSize() const
{
	return FVector2D(MGFXMaterial->BaseCanvasSize);
}

TArray<TObjectPtr<UMGFXMaterialLayer>> FMGFXMaterialEditor::GetSelectedLayers() const
{
	return SelectedLayers;
}

void FMGFXMaterialEditor::SetSelectedLayers(const TArray<UMGFXMaterialLayer*>& Layers)
{
	if (SelectedLayers == Layers)
	{
		return;
	}

	SelectedLayers = Layers;

	if (SelectedLayers.IsEmpty())
	{
		// default to inspecting material when nothing or multiple things are selected
		DetailsView->SetObject(MGFXMaterial);
	}
	else
	{
		TArray<UObject*> Objects;
		Algo::Transform(SelectedLayers, Objects, [](UMGFXMaterialLayer* Layer) { return Layer; });
		DetailsView->SetObjects(Objects);
	}

	OnLayerSelectionChangedEvent.Broadcast(SelectedLayers);
}

void FMGFXMaterialEditor::ClearSelectedLayers()
{
	SetSelectedLayers(TArray<UMGFXMaterialLayer*>());
}

void FMGFXMaterialEditor::OnLayerSelectionChanged(TArray<TObjectPtr<UMGFXMaterialLayer>> TreeSelectedLayers)
{
	check(DetailsView.IsValid());

	SetSelectedLayers(TreeSelectedLayers);
}

bool FMGFXMaterialEditor::IsDetailsPropertyVisible(const FPropertyAndParent& PropertyAndParent)
{
	const TArray<FName> CategoriesToHide = {
		FName("Shape|Editor")
	};

	const FProperty* Property = PropertyAndParent.ParentProperties.Num() > 0 ? PropertyAndParent.ParentProperties.Last() : &PropertyAndParent.Property;

	// get the topmost parent's category name if the property has one
	const FString CategoryString = FObjectEditorUtils::GetCategoryFName(Property).ToString();
	FString SubcategoryString = CategoryString;

	int32 SubcategoryStart;
	if (CategoryString.FindChar(TEXT('|'), SubcategoryStart))
	{
		SubcategoryString = CategoryString.Left(SubcategoryStart);
	}

	const bool bIsHiddenCategory = CategoriesToHide.ContainsByPredicate([&CategoryString, &SubcategoryString](const FName& Element)
	{
		const FString ElementString = Element.ToString();
		return CategoryString == ElementString || SubcategoryString == ElementString;
	});

	if (bIsHiddenCategory)
	{
		return false;
	}

	return true;
}

bool FMGFXMaterialEditor::IsDetailsRowVisible(FName InRowName, FName InParentName)
{
	const TArray<FName> CategoriesToHide = {
		FName("Shape|Editor")
	};

	if (CategoriesToHide.Contains(InParentName))
	{
		return false;
	}
	return true;
}

void FMGFXMaterialEditor::OnLayersChanged()
{
	RegeneratePreviewMaterial();

	OnLayersChangedEvent.Broadcast();
}

void FMGFXMaterialEditor::OnLayersChangedByLayersView()
{
	OnLayersChanged();
}

void FMGFXMaterialEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(MGFXMaterial);
	Collector.AddReferencedObject(PreviewMaterial);
	Collector.AddReferencedObject(PreviewMID);
}

void FMGFXMaterialEditor::NotifyPreChange(FProperty* PropertyAboutToChange)
{
}

void FMGFXMaterialEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{
	if (!PropertyThatChanged || !PropertyThatChanged->GetActiveNode())
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();

	// handle simple properties
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterial, Material))
	{
		OnMaterialAssetChanged();
		return;
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterial, DesignerBackground) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterial, bOverrideDesignerBackground) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterial, bAllAnimatable))
	{
		return;
	}

	// handle material parameter properties

	// we could set the default values on the Material's expressions here, and it would update fairly responsively,
	// but it's not perfect, and flickering occurs. Instead, we modify the MID which has perfect responsiveness
	// during Interactive property changes and then update the Material during ValueSet changes.

	// the preview material is always generated without optimization, so all params will be available
	const bool bInteractive = PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive;
	bool bIsParameterChange = false;

	for (int32 Idx = 0; Idx < PropertyChangedEvent.GetNumObjectsBeingEdited(); ++Idx)
	{
		const UObject* EditedObject = PropertyChangedEvent.GetObjectBeingEdited(Idx);
		if (const UMGFXMaterialLayer* EditedLayer = Cast<UMGFXMaterialLayer>(EditedObject))
		{
			const FString ParamPrefix = EditedLayer->Name + ".";

			// editing a layer transform
			if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterialLayer, Transform))
			{
				if (TDoubleLinkedList<FProperty*>::TDoubleLinkedListNode* ParentNode = PropertyThatChanged->GetActiveNode()->GetPrevNode())
				{
					const FName ParentPropertyName = ParentNode->GetValue()->GetFName();
					if (PropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Location))
					{
						// setting Location
						SetMaterialScalarParameterValue(FName(ParamPrefix + Generator->Param_LocationX), EditedLayer->Transform.Location.X, bInteractive);
						SetMaterialScalarParameterValue(FName(ParamPrefix + Generator->Param_LocationY), EditedLayer->Transform.Location.Y, bInteractive);
						bIsParameterChange = true;
					}
					else if (PropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Rotation))
					{
						// setting Rotation
						const float NewValue = EditedLayer->Transform.Rotation;
						const FString ParamName = Generator->Param_Rotation;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
						bIsParameterChange = true;
					}
					else if (PropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Scale))
					{
						// setting Scale
						SetMaterialScalarParameterValue(FName(ParamPrefix + Generator->Param_ScaleX), EditedLayer->Transform.Scale.X, bInteractive);
						SetMaterialScalarParameterValue(FName(ParamPrefix + Generator->Param_ScaleY), EditedLayer->Transform.Scale.Y, bInteractive);
						bIsParameterChange = true;
					}
					else if (ParentPropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Location))
					{
						// setting X or Y of Location
						const bool bIsX = PropertyName == GET_MEMBER_NAME_CHECKED(FVector2f, X);
						const float NewValue = bIsX ? EditedLayer->Transform.Location.X : EditedLayer->Transform.Location.Y;
						const FString ParamName = bIsX ? Generator->Param_LocationX : Generator->Param_LocationY;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
						bIsParameterChange = true;
					}
					else if (ParentPropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Scale))
					{
						// setting X or Y of Scale
						const bool bIsX = PropertyName == GET_MEMBER_NAME_CHECKED(FVector2f, X);
						const float NewValue = bIsX ? EditedLayer->Transform.Scale.X : EditedLayer->Transform.Scale.Y;
						const FString ParamName = bIsX ? Generator->Param_ScaleX : Generator->Param_ScaleY;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
						bIsParameterChange = true;
					}
				}
			}
		}
		else if (const UMGFXMaterialShape* EditedShape = Cast<UMGFXMaterialShape>(EditedObject))
		{
			if (const UMGFXMaterialLayer* OwningLayer = Cast<UMGFXMaterialLayer>(EditedShape->GetOuter()))
			{
				const FString ParamPrefix = OwningLayer->Name + ".";

				// editing a shape
				for (const FMGFXMaterialShapeInput& Input : EditedShape->GetInputs())
				{
					// member property name is all that matters, the entire parameter will be updated
					if (MemberPropertyName == Input.Name)
					{
						switch (Input.Type)
						{
						case EMGFXMaterialShapeInputType::Float:
							SetMaterialScalarParameterValue(FName(ParamPrefix + Input.Name), Input.Value.R, bInteractive);
							bIsParameterChange = true;
							break;
						case EMGFXMaterialShapeInputType::Vector2:
							SetMaterialScalarParameterValue(FName(ParamPrefix + Input.Name + "X"), Input.Value.R, bInteractive);
							SetMaterialScalarParameterValue(FName(ParamPrefix + Input.Name + "Y"), Input.Value.G, bInteractive);
							bIsParameterChange = true;
							break;
						case EMGFXMaterialShapeInputType::Vector3:
						case EMGFXMaterialShapeInputType::Vector4:
							SetMaterialVectorParameterValue(FName(ParamPrefix + Input.Name), Input.Value, bInteractive);
							bIsParameterChange = true;
							break;
						default: ;
						}
					}
				}
			}
		}
		else if (const UMGFXMaterialShapeFill* EditedFill = Cast<UMGFXMaterialShapeFill>(EditedObject))
		{
			// editing fill visual
			if (const UMGFXMaterialLayer* OwningLayer = Cast<UMGFXMaterialLayer>(EditedFill->GetOuter()->GetOuter()))
			{
				const FString ParamPrefix = OwningLayer->Name + ".";

				if (PropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterialShapeFill, Color))
				{
					SetMaterialVectorParameterValue(FName(ParamPrefix + "Color"), EditedFill->Color, bInteractive);
					bIsParameterChange = true;
				}
			}
		}
		else if (const UMGFXMaterialShapeStroke* EditedStroke = Cast<UMGFXMaterialShapeStroke>(EditedObject))
		{
			if (const UMGFXMaterialLayer* OwningLayer = Cast<UMGFXMaterialLayer>(EditedStroke->GetOuter()->GetOuter()))
			{
				const FString ParamPrefix = OwningLayer->Name + ".";

				// editing stroke visual
				if (PropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterialShapeStroke, Color))
				{
					SetMaterialVectorParameterValue(FName(ParamPrefix + "Color"), EditedStroke->Color, bInteractive);
					bIsParameterChange = true;
				}
				else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterialShapeStroke, StrokeWidth))
				{
					SetMaterialScalarParameterValue(FName(ParamPrefix + "StrokeWidth"), EditedStroke->StrokeWidth, bInteractive);
					bIsParameterChange = true;
				}
			}
		}
	}

	if (!bInteractive)
	{
		// TODO: more accurate filtering of properties that should cause regenerate
		const bool bShouldRegenerate = !bIsParameterChange;

		if (bShouldRegenerate)
		{
			RegeneratePreviewMaterial();
		}
	}
}

void FMGFXMaterialEditor::SetMaterialScalarParameterValue(FName ParameterName, float Value, bool bInteractive) const
{
	PreviewMID->Modify();
	PreviewMID->SetScalarParameterValue(ParameterName, Value);
}

void FMGFXMaterialEditor::SetMaterialVectorParameterValue(FName ParameterName, FLinearColor Value, bool bInteractive) const
{
	PreviewMID->Modify();
	PreviewMID->SetVectorParameterValue(ParameterName, Value);
}

bool FMGFXMaterialEditor::MatchesContext(const FTransactionContext& InContext,
                                         const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjects) const
{
	for (const TPair<UObject*, FTransactionObjectEvent>& TransactionObjectPair : TransactionObjects)
	{
		if (TransactionObjectPair.Value.HasPendingKillChange())
		{
			return true;
		}

		const UObject* Object = TransactionObjectPair.Key;
		while (Object != nullptr)
		{
			if (Object->IsA<UMGFXMaterial>() ||
				Object->IsA<UMGFXMaterialLayer>() ||
				Object->IsA<UMGFXMaterialShape>() ||
				Object->IsA<UMGFXMaterialShapeVisual>())
			{
				return true;
			}

			Object = Object->GetOuter();
		}
	}

	return false;
}

void FMGFXMaterialEditor::PostUndo(bool bSuccess)
{
	OnLayersChangedEvent.Broadcast();
}

void FMGFXMaterialEditor::PostRedo(bool bSuccess)
{
	OnLayersChangedEvent.Broadcast();
}

void FMGFXMaterialEditor::BindCommands()
{
	const FMGFXMaterialEditorCommands& Commands = FMGFXMaterialEditorCommands::Get();
	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(
		Commands.Apply,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::Apply));

	UICommandList->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::DeleteSelectedLayers),
		FCanExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CanDeleteSelectedLayers));

	UICommandList->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CopySelectedLayers),
		FCanExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CanCopySelectedLayers));

	UICommandList->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CutSelectedLayers),
		FCanExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CanCutSelectedLayers));

	UICommandList->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::PasteLayers),
		FCanExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CanPasteLayers));

	UICommandList->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::DuplicateSelectedLayers),
		FCanExecuteAction::CreateSP(this, &FMGFXMaterialEditor::CanDuplicateSelectedLayers));
}

void FMGFXMaterialEditor::RegisterToolbar()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* ToolBar;
	FName ParentName;
	const FName MenuName = GetToolMenuToolbarName(ParentName);
	if (ToolMenus->IsMenuRegistered(MenuName))
	{
		ToolBar = ToolMenus->ExtendMenu(MenuName);
	}
	else
	{
		ToolBar = ToolMenus->RegisterMenu(MenuName, ParentName, EMultiBoxType::ToolBar);
	}

	const FMGFXMaterialEditorCommands& MGFXEditorCommands = FMGFXMaterialEditorCommands::Get();
	const FToolMenuInsert InsertAfterAssetSection("Asset", EToolMenuInsertType::After);
	{
		FToolMenuSection& Section = ToolBar->AddSection("MGFXToolbar", TAttribute<FText>(), InsertAfterAssetSection);
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(MGFXEditorCommands.Apply));
	}
}


TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Canvas(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			SAssignNew(CanvasWidget, SMGFXMaterialEditorCanvas)
			.MGFXMaterialEditor(SharedThis(this))
		];
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Layers(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			SAssignNew(LayersWidget, SMGFXMaterialEditorLayers)
			.MGFXMaterialEditor(SharedThis(this))
			.OnSelectionChanged(this, &FMGFXMaterialEditor::OnLayerSelectionChanged)
			.OnLayersChanged(this, &FMGFXMaterialEditor::OnLayersChangedByLayersView)
		];
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			DetailsView.ToSharedRef()
		];
}

void FMGFXMaterialEditor::OnMaterialAssetChanged()
{
	OnMaterialChangedEvent.Broadcast(MGFXMaterial->Material);
}

void FMGFXMaterialEditor::CreatePreviewMaterials()
{
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	check(MaterialFactory);

	PreviewMaterial = Cast<UPreviewMaterial>(MaterialFactory->FactoryCreateNew(
		UPreviewMaterial::StaticClass(),
		GetTransientPackage(),
		NAME_None,
		RF_Transactional | RF_Transient,
		nullptr,
		GWarn
	));

	PreviewMaterial->bIsPreviewMaterial = true;

	// create an MID of the preview for interactive parameter changes
	PreviewMID = UMaterialInstanceDynamic::Create(PreviewMaterial, nullptr);
	PreviewMID->SetFlags(RF_Transactional | RF_Transient);
	OnPreviewMaterialChangedEvent.Broadcast(PreviewMID);
}

IMaterialEditor* FMGFXMaterialEditor::FindMaterialEditor(UMaterial* Material)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	IAssetEditorInstance* AssetEditorInstance = AssetEditorSubsystem->FindEditorForAsset(Material, false);
	if (!AssetEditorInstance || AssetEditorInstance->GetEditorName() != "MaterialEditor")
	{
		return nullptr;
	}

	return static_cast<IMaterialEditor*>(AssetEditorInstance);
}

UPreviewMaterial* FMGFXMaterialEditor::GetMaterialEditorPreviewMaterial(IMaterialEditor* MaterialEditor)
{
	if (!MaterialEditor)
	{
		return nullptr;
	}

	// only a toolkit menu context can access the editing objects of a toolkit
	UAssetEditorToolkitMenuContext* const EditorContext = NewObject<UAssetEditorToolkitMenuContext>();
	EditorContext->Toolkit = MaterialEditor->AsWeak();

	// search for the material (not the original one) from the list of EditingObjects
	TArray<UObject*> MaterialEditingObjects = EditorContext->GetEditingObjects();
	for (UObject* EditingObject : MaterialEditingObjects)
	{
		if (UPreviewMaterial* EditingMaterial = Cast<UPreviewMaterial>(EditingObject))
		{
			// assume this is the right material (there should be only 1 other)
			return EditingMaterial;
		}
	}

	return nullptr;
}

void FMGFXMaterialEditor::DeleteSelectedLayers()
{
	FScopedTransaction Transaction(LOCTEXT("DeleteLayer", "Delete Layer"));

	MGFXMaterial->Modify();

	TArray<UMGFXMaterialLayer*> LayersToDelete = SelectedLayers;

	ClearSelectedLayers();

	for (UMGFXMaterialLayer* Layer : LayersToDelete)
	{
		if (Layer->GetParentLayer())
		{
			Layer->GetParentLayer()->RemoveLayer(Layer);
		}
		else
		{
			MGFXMaterial->RemoveLayer(Layer);
		}
	}

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanDeleteSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::CopySelectedLayers()
{
	const FString ExportedText = FMGFXMaterialEditorUtils::CopyLayers(SelectedLayers);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FMGFXMaterialEditor::CanCopySelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::CutSelectedLayers()
{
	CopySelectedLayers();
	DeleteSelectedLayers();
}

bool FMGFXMaterialEditor::CanCutSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::PasteLayers()
{
	// paste into the first selected layer, or as a root layer
	UObject* ContainerObject = SelectedLayers.IsEmpty() ? MGFXMaterial : Cast<UObject>(SelectedLayers[0]);
	IMGFXMaterialLayerParentInterface* Container = CastChecked<IMGFXMaterialLayerParentInterface>(ContainerObject);

	FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

	// grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	bool bSuccess = true;
	const TArray<UMGFXMaterialLayer*> PastedLayers = FMGFXMaterialEditorUtils::PasteLayers(MGFXMaterial, TextToImport, Container, bSuccess);
	if (!bSuccess)
	{
		Transaction.Cancel();
		return;
	}

	SetSelectedLayers(PastedLayers);

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanPasteLayers()
{
	return true;
}

void FMGFXMaterialEditor::DuplicateSelectedLayers()
{
	check(!SelectedLayers.IsEmpty());

	// always duplicating into the parent of the first topmost layer
	const TArray<UMGFXMaterialLayer*> TopmostLayers = FMGFXMaterialEditorUtils::GetTopmostLayers(SelectedLayers);
	check(!TopmostLayers.IsEmpty());

	IMGFXMaterialLayerParentInterface* Container = TopmostLayers[0]->GetParentContainer();

	const FString ExportedText = FMGFXMaterialEditorUtils::CopyLayers(SelectedLayers);

	FScopedTransaction Transaction(FGenericCommands::Get().Duplicate->GetDescription());
	bool bSuccess = true;
	const TArray<UMGFXMaterialLayer*> DuplicatedLayers = FMGFXMaterialEditorUtils::PasteLayers(MGFXMaterial, ExportedText, Container, bSuccess);
	if (!bSuccess)
	{
		Transaction.Cancel();
		return;
	}

	SetSelectedLayers(DuplicatedLayers);

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanDuplicateSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}


#undef LOCTEXT_NAMESPACE
