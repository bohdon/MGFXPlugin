// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "AssetToolsModule.h"
#include "IMaterialEditor.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MGFXMaterialLayer.h"
#include "MGFXPropertyMacros.h"
#include "ObjectEditorUtils.h"
#include "SMGFXMaterialEditorCanvas.h"
#include "SMGFXMaterialEditorLayers.h"
#include "Factories/MaterialFactoryNew.h"
#include "Framework/Commands/GenericCommands.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/TransactionObjectEvent.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Shapes/MGFXMaterialShapeVisual.h"
#include "UObject/PropertyAccessUtil.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


constexpr auto ReadOnlyFlags = PropertyAccessUtil::EditorReadOnlyFlags;
constexpr auto NotifyMode = EPropertyAccessChangeNotifyMode::Default;

const FName FMGFXMaterialEditor::CanvasTabId(TEXT("MGFXMaterialEditorCanvasTab"));
const FName FMGFXMaterialEditor::LayersTabId(TEXT("MGFXMaterialEditorLayersTab"));
const FName FMGFXMaterialEditor::DetailsTabId(TEXT("MGFXMaterialEditorDetailsTab"));
const FName FMGFXMaterialEditor::Reroute_CanvasUVs(TEXT("CanvasUVs"));
const FName FMGFXMaterialEditor::Reroute_CanvasFilterWidth(TEXT("CanvasFilterWidth"));
const FName FMGFXMaterialEditor::Reroute_LayersOutput(TEXT("LayersOutput"));
const int32 FMGFXMaterialEditor::GridSize(16);
const FString FMGFXMaterialEditor::Param_LocationX(TEXT("LocationX"));
const FString FMGFXMaterialEditor::Param_LocationY(TEXT("LocationY"));
const FString FMGFXMaterialEditor::Param_Rotation(TEXT("Rotation"));
const FString FMGFXMaterialEditor::Param_ScaleX(TEXT("ScaleX"));
const FString FMGFXMaterialEditor::Param_ScaleY(TEXT("ScaleY"));


FMGFXMaterialEditor::FMGFXMaterialEditor()
	: NodePosBaselineLeft(GridSize * -64),
	  SDFRerouteColor(FLinearColor(0.f, 0.f, 0.f)),
	  RGBARerouteColor(FLinearColor(0.22f, .09f, 0.55f)),
	  bAutoRegenerate(true)
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
	UpdatePreviewMaterial();

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

UMaterial* FMGFXMaterialEditor::GetGeneratedMaterial() const
{
	check(MGFXMaterial);
	return MGFXMaterial->Material;
}

void FMGFXMaterialEditor::RegenerateMaterial()
{
	UMaterial* Material = GetGeneratedMaterial();
	if (!Material)
	{
		Material = CreateMaterialAsset();

		if (!Material)
		{
			UE_LOG(LogMGFXEditor, Error, TEXT("Failed to create Material for %s"), *GetNameSafe(MGFXMaterial));
			return;
		}
	}

	// TODO: close material editor, or find a way to refresh it, could just copy (manual duplicate) to the preview material?

	const FScopedTransaction Transaction(LOCTEXT("RegenerateMaterial", "MGFX Material Editor: Regenerate Material"));

	Material->Modify();

	// set material properties
	Material->MaterialDomain = MGFXMaterial->MaterialDomain;
	Material->BlendMode = MGFXMaterial->BlendMode;

	FMGFXMaterialBuilder Builder(Material);

	Generate_DeleteAllNodes(Builder);
	Generate_AddWarningComment(Builder);
	Generate_AddUVsBoilerplate(Builder);
	Generate_Layers(Builder);

	FVector2D NodePos(GridSize * -31, 0);

	if (MGFXMaterial->OutputProperty == MP_EmissiveColor)
	{
		// connect layer output to final color
		UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(NodePos, Reroute_LayersOutput);
		Builder.ConnectProperty(OutputUsageExp, "", MGFXMaterial->OutputProperty);
	}
	else
	{
		// connect constant to final color
		UMaterialExpressionConstant3Vector* ColorExp = Builder.Create<UMaterialExpressionConstant3Vector>(NodePos);
		SET_PROP(ColorExp, Constant, MGFXMaterial->DefaultEmissiveColor);
		Builder.ConnectProperty(ColorExp, "", MP_EmissiveColor);

		NodePos.Y += GridSize * 15;

		// connect layer output to custom property
		UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(NodePos, Reroute_LayersOutput);
		Builder.ConnectProperty(OutputUsageExp, "", MGFXMaterial->OutputProperty);
	}

	Builder.RecompileMaterial();

	UpdatePreviewMaterial();

	// TODO: use events to keep this up to date
	// update canvas artboard
	CanvasWidget->UpdateArtboardSize();
}

void FMGFXMaterialEditor::ToggleAutoRegenerate()
{
	bAutoRegenerate = !bAutoRegenerate;
}

UMaterial* FMGFXMaterialEditor::CreateMaterialAsset()
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
	RegenerateMaterial();

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
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterial, bOverrideDesignerBackground))
	{
		return;
	}

	// handle material parameter properties

	// we could set the default values on the Material's expressions here, and it would update fairly responsively,
	// but it's not perfect, and flickering occurs. Instead we modify the MID which has perfect responsiveness, and any
	// temporary modifications will be correctly replaced once the material is generated.

	// this unfortunately only works if the params are not optimized out, so it's recommended to work without
	// optimizations enabled, then enable them afterwards.
	const bool bInteractive = PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive;

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
						SetMaterialScalarParameterValue(FName(ParamPrefix + Param_LocationX), EditedLayer->Transform.Location.X, bInteractive);
						SetMaterialScalarParameterValue(FName(ParamPrefix + Param_LocationY), EditedLayer->Transform.Location.Y, bInteractive);
					}
					else if (PropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Rotation))
					{
						// setting Rotation
						const float NewValue = EditedLayer->Transform.Rotation;
						const FString ParamName = Param_Rotation;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
					}
					else if (PropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Scale))
					{
						// setting Scale
						SetMaterialScalarParameterValue(FName(ParamPrefix + Param_ScaleX), EditedLayer->Transform.Scale.X, bInteractive);
						SetMaterialScalarParameterValue(FName(ParamPrefix + Param_ScaleY), EditedLayer->Transform.Scale.Y, bInteractive);
					}
					else if (ParentPropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Location))
					{
						// setting X or Y of Location
						const bool bIsX = PropertyName == GET_MEMBER_NAME_CHECKED(FVector2f, X);
						const float NewValue = bIsX ? EditedLayer->Transform.Location.X : EditedLayer->Transform.Location.Y;
						const FString ParamName = bIsX ? Param_LocationX : Param_LocationY;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
					}
					else if (ParentPropertyName == GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Scale))
					{
						// setting X or Y of Scale
						const bool bIsX = PropertyName == GET_MEMBER_NAME_CHECKED(FVector2f, X);
						const float NewValue = bIsX ? EditedLayer->Transform.Scale.X : EditedLayer->Transform.Scale.Y;
						const FString ParamName = bIsX ? Param_ScaleX : Param_ScaleY;

						SetMaterialScalarParameterValue(FName(ParamPrefix + ParamName), NewValue, bInteractive);
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
							break;
						case EMGFXMaterialShapeInputType::Vector2:
							SetMaterialScalarParameterValue(FName(ParamPrefix + Input.Name + "X"), Input.Value.R, bInteractive);
							SetMaterialScalarParameterValue(FName(ParamPrefix + Input.Name + "Y"), Input.Value.G, bInteractive);
							break;
						case EMGFXMaterialShapeInputType::Vector3:
						case EMGFXMaterialShapeInputType::Vector4:
							SetMaterialVectorParameterValue(FName(ParamPrefix + Input.Name), Input.Value, bInteractive);
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
				}
				else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMGFXMaterialShapeStroke, StrokeWidth))
				{
					SetMaterialScalarParameterValue(FName(ParamPrefix + "StrokeWidth"), EditedStroke->StrokeWidth, bInteractive);
				}
			}
		}
	}

	if (!bInteractive)
	{
		// any interactive preview changes should be cleared now
		UpdatePreviewMaterial();
	}

	if (bAutoRegenerate && PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
	{
		// TODO: more accurate filtering of properties that should cause regenerate
		RegenerateMaterial();
	}
}

void FMGFXMaterialEditor::SetMaterialScalarParameterValue(FName PropertyName, float Value, bool bInteractive) const
{
	if (bInteractive)
	{
		PreviewMaterial->SetScalarParameterValue(PropertyName, Value);
	}
	else
	{
		FMGFXMaterialBuilder Builder(GetGeneratedMaterial());
		Builder.Material->Modify();
		Builder.SetScalarParameterValue(PropertyName, Value);
	}
}

void FMGFXMaterialEditor::SetMaterialVectorParameterValue(FName PropertyName, FLinearColor Value, bool bInteractive) const
{
	if (bInteractive)
	{
		PreviewMaterial->SetVectorParameterValue(PropertyName, Value);
	}
	else
	{
		FMGFXMaterialBuilder Builder(GetGeneratedMaterial());
		Builder.Material->Modify();
		Builder.SetVectorParameterValue(PropertyName, Value);
	}
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
	UpdatePreviewMaterial();
}

void FMGFXMaterialEditor::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

void FMGFXMaterialEditor::BindCommands()
{
	const FMGFXMaterialEditorCommands& Commands = FMGFXMaterialEditorCommands::Get();
	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(
		Commands.RegenerateMaterial,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::RegenerateMaterial));

	UICommandList->MapAction(
		Commands.ToggleAutoRegenerate,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::ToggleAutoRegenerate),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FMGFXMaterialEditor::IsAutoRegenerateEnabled));

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
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(MGFXEditorCommands.RegenerateMaterial));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(MGFXEditorCommands.ToggleAutoRegenerate));
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

	UpdatePreviewMaterial();
}

void FMGFXMaterialEditor::UpdatePreviewMaterial()
{
	// create if it doesn't already exist
	if (!PreviewMaterial && MGFXMaterial->Material)
	{
		PreviewMaterial = UMaterialInstanceDynamic::Create(MGFXMaterial->Material, nullptr);
		OnPreviewMaterialChangedEvent.Broadcast(PreviewMaterial);
	}
	// reset values
	if (PreviewMaterial)
	{
		PreviewMaterial->ClearParameterValues();
	}
}

void FMGFXMaterialEditor::Generate_DeleteAllNodes(FMGFXMaterialBuilder& Builder)
{
	Builder.DeleteAll();
}

void FMGFXMaterialEditor::Generate_AddWarningComment(FMGFXMaterialBuilder& Builder)
{
	const FLinearColor CommentColor = FLinearColor(0.06f, 0.02f, 0.02f);
	const FString Text = FString::Printf(TEXT("Generated by %s\nDo not edit manually"), *GetNameSafe(MGFXMaterial));

	FVector2D NodePos(GridSize * -20, GridSize * -10);
	UMaterialExpressionComment* CommentExp = Builder.CreateComment(NodePos, Text, CommentColor);
}

void FMGFXMaterialEditor::Generate_AddUVsBoilerplate(FMGFXMaterialBuilder& Builder)
{
	// temp var for easier node positioning
	FVector2D NodePosOffset = FVector2D::Zero();

	FVector2D NodePos(GridSize * -64, GridSize * 30);

	// create CanvasWidth and CanvasHeight scalar parameters
	const FName ParamGroup("Canvas");
	auto* CanvasAppendExp = Generate_Vector2Parameter(Builder, NodePos, MGFXMaterial->BaseCanvasSize,
	                                                  FString(), ParamGroup, 0, "CanvasWidth", "CanvasHeight");

	NodePos.X += GridSize * 15;

	// create GetUserInterfaceUVs material function
	NodePosOffset = FVector2D(0, GridSize * 8);
	UMaterialExpressionMaterialFunctionCall* InterfaceUVsExp = Builder.CreateFunction(
		NodePos + NodePosOffset, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/Engine/Functions/UserInterface/GetUserInterfaceUV.GetUserInterfaceUV")));

	NodePos.X += GridSize * 15;

	// multiply Normalized UV by canvas size
	UMaterialExpressionMultiply* MultiplyExp = Builder.Create<UMaterialExpressionMultiply>(NodePos);
	Builder.Connect(CanvasAppendExp, "", MultiplyExp, "A");
	Builder.Connect(InterfaceUVsExp, "Normalized UV", MultiplyExp, "B");

	NodePos.X += GridSize * 15;

	// output to canvas UVs reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = Builder.CreateNamedReroute(NodePos, Reroute_CanvasUVs);
	Builder.Connect(MultiplyExp, "", OutputRerouteExp, "");

	NodePos.X += GridSize * 15;

	// compute common filter width to use for all shapes based on canvas resolution
	UMaterialExpression* FilterWidthExp = nullptr;
	// the output from the filter width expression to use, which may vary
	FString FilterWidthOutput = FString();

	if (MGFXMaterial->bComputeFilterWidth)
	{
		// create bias constant (even if its unused)
		UMaterialExpressionConstant* FilterWidthBiasExp = Builder.Create<UMaterialExpressionConstant>(NodePos + FVector2D(0, GridSize * 8));
		SET_PROP(FilterWidthBiasExp, R, MGFXMaterial->FilterWidthBias);

		NodePos.X += GridSize * 15;

		// create filter width func
		UMaterialExpressionMaterialFunctionCall* FilterWidthFuncExp = Builder.CreateFunction(
			NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_FilterWidth.MF_MGFX_FilterWidth")));
		Builder.Connect(OutputRerouteExp, "", FilterWidthFuncExp, "");
		Builder.Connect(FilterWidthBiasExp, "", FilterWidthFuncExp, "Bias");

		// only use biased output if not zero, otherwise optimize out
		if (!FMath::IsNearlyZero(MGFXMaterial->FilterWidthBias))
		{
			FilterWidthOutput = FString("Biased");
		}

		NodePos.X += GridSize * 15;

		// apply filter width scale if not 1x
		if (!FMath::IsNearlyEqual(MGFXMaterial->FilterWidthScale, 1.f))
		{
			UMaterialExpressionMultiply* FilterWidthScaleExp = Builder.Create<UMaterialExpressionMultiply>(NodePos);
			Builder.Connect(FilterWidthFuncExp, FilterWidthOutput, FilterWidthScaleExp, "");
			SET_PROP(FilterWidthScaleExp, ConstB, MGFXMaterial->FilterWidthScale);

			NodePos.X += GridSize * 15;

			FilterWidthExp = FilterWidthScaleExp;
			FilterWidthOutput = "";
		}
		else
		{
			FilterWidthExp = FilterWidthFuncExp;
		}
	}
	else
	{
		UMaterialExpressionConstant* FilterWidthConstExp = Builder.Create<UMaterialExpressionConstant>(NodePos);
		SET_PROP(FilterWidthConstExp, R, MGFXMaterial->FilterWidthScale);

		NodePos.X += GridSize * 15;

		FilterWidthExp = FilterWidthConstExp;
	}

	// add filter width reroute
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthRerouteExp = Builder.CreateNamedReroute(NodePos, Reroute_CanvasFilterWidth);
	Builder.Connect(FilterWidthExp, FilterWidthOutput, FilterWidthRerouteExp, "");
}

void FMGFXMaterialEditor::Generate_Layers(FMGFXMaterialBuilder& Builder)
{
	FVector2D NodePos(NodePosBaselineLeft, GridSize * 80);

	// start with background color
	UMaterialExpressionConstant4Vector* BGColorExp = Builder.Create<UMaterialExpressionConstant4Vector>(NodePos);
	SET_PROP(BGColorExp, Constant, FLinearColor::Black);

	NodePos.X += GridSize * 15;

	// create base UVs
	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = Builder.FindNamedReroute(Reroute_CanvasUVs);
	UMaterialExpressionNamedRerouteDeclaration* CanvasFilterWidthDeclaration = Builder.FindNamedReroute(Reroute_CanvasFilterWidth);
	check(CanvasUVsDeclaration);
	check(CanvasFilterWidthDeclaration);

	const FMGFXMaterialUVsAndFilterWidth BGUVsInfo(CanvasUVsDeclaration, CanvasFilterWidthDeclaration);

	const FMGFXMaterialLayerOutputs BaseLayerOutputs(BGUVsInfo, nullptr, BGColorExp);

	// generate all layers recursively
	FMGFXMaterialLayerOutputs LayerOutputs = BaseLayerOutputs;
	for (int32 Idx = MGFXMaterial->RootLayers.Num() - 1; Idx >= 0; --Idx)
	{
		const UMGFXMaterialLayer* ChildLayer = MGFXMaterial->RootLayers[Idx];

		LayerOutputs = Generate_Layer(Builder, NodePos, ChildLayer, BaseLayerOutputs.UVs, LayerOutputs);
	}

	NodePos.X += GridSize * 15;

	// connect to layers output reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = Builder.CreateNamedReroute(NodePos, Reroute_LayersOutput, RGBARerouteColor);
	Builder.Connect(LayerOutputs.VisualExp, "", OutputRerouteExp, "");
}

FMGFXMaterialLayerOutputs FMGFXMaterialEditor::Generate_Layer(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialLayer* Layer,
                                                              const FMGFXMaterialUVsAndFilterWidth& UVs, const FMGFXMaterialLayerOutputs& PrevOutputs)
{
	// collect expressions for this layers output
	FMGFXMaterialLayerOutputs LayerOutputs;

	const FString LayerName = Layer->Name.IsEmpty() ? Layer->GetName() : Layer->Name;
	const FString ParamPrefix = LayerName + ".";
	const FName ParamGroup = FName(FString::Printf(TEXT("%s"), *LayerName));

	// reset baseline and move to new line
	NodePos.X = NodePosBaselineLeft;
	NodePos.Y += GridSize * 40;


	// generate transform uvs
	UMaterialExpressionNamedRerouteUsage* ParentUVsUsageExp = Builder.CreateNamedRerouteUsage(NodePos, UVs.UVsExp);

	NodePos.X += GridSize * 15;

	// apply layer transform. this may just return the parent uvs if optimized out
	UMaterialExpression* UVsExp = Generate_TransformUVs(Builder, NodePos, Layer->Transform, ParentUVsUsageExp, ParamPrefix, ParamGroup);

	// store as a reroute for any children
	const bool bCreateReroute = Layer->HasLayers();
	if (bCreateReroute)
	{
		// output to named reroute
		LayerOutputs.UVs.UVsExp = Builder.CreateNamedReroute(NodePos, FName(ParamPrefix + "UVs"));
		Builder.Connect(UVsExp, "", LayerOutputs.UVs.UVsExp, "");
		// treat this as the uvs expression for layer
		UVsExp = LayerOutputs.UVs.UVsExp;

		NodePos.X += GridSize * 15;
	}
	else
	{
		// reuse parent UVs
		LayerOutputs.UVs.UVsExp = UVs.UVsExp;
	}

	// recalculate filter width to use for these uvs if the SDF gradient can ever be scaled
	const bool bNoOptimization = Layer->Transform.bAnimatable || MGFXMaterial->bAllAnimatable;
	const bool bHasModifiedScale = !(Layer->Transform.Scale - FVector2f::One()).IsNearlyZero();
	if (bNoOptimization || bHasModifiedScale)
	{
		UMaterialExpressionMaterialFunctionCall* UVFilterFuncExp = Builder.CreateFunction(
			NodePos + FVector2D(0, GridSize * 8),
			TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_FilterWidth.MF_MGFX_FilterWidth")));
		Builder.Connect(UVsExp, "", UVFilterFuncExp, "");

		NodePos.X += GridSize * 15;

		LayerOutputs.UVs.FilterWidthExp = Builder.CreateNamedReroute(NodePos + FVector2D(0, GridSize * 8), FName(ParamPrefix + "FilterWidth"));
		Builder.Connect(UVFilterFuncExp, "", LayerOutputs.UVs.FilterWidthExp, "");

		NodePos.X += GridSize * 15;
	}
	else
	{
		// reuse parent filter width
		LayerOutputs.UVs.FilterWidthExp = UVs.FilterWidthExp;
	}

	// generate children layers bottom to top
	FMGFXMaterialLayerOutputs ChildOutputs = LayerOutputs;
	for (int32 Idx = Layer->NumLayers() - 1; Idx >= 0; --Idx)
	{
		const UMGFXMaterialLayer* ChildLayer = Layer->GetLayer(Idx);

		ChildOutputs = Generate_Layer(Builder, NodePos, ChildLayer, LayerOutputs.UVs, ChildOutputs);
	}


	// generate shape for this layer
	if (Layer->Shape)
	{
		// the shape, with an SDF output
		LayerOutputs.ShapeExp = Generate_Shape(Builder, NodePos, Layer->Shape, UVsExp, ParamPrefix, ParamGroup);

		// the shape visuals, including all fills and strokes
		LayerOutputs.VisualExp = Generate_ShapeVisuals(Builder, NodePos, Layer->Shape, LayerOutputs.ShapeExp,
		                                               LayerOutputs.UVs.FilterWidthExp, ParamPrefix, ParamGroup);
	}

	// merge this layer with its children
	if (Layer->HasLayers())
	{
		if (ChildOutputs.VisualExp && ChildOutputs.VisualExp != LayerOutputs.VisualExp)
		{
			if (LayerOutputs.VisualExp)
			{
				LayerOutputs.VisualExp = Generate_MergeVisual(Builder, NodePos, LayerOutputs.VisualExp, ChildOutputs.VisualExp,
				                                              Layer->MergeOperation, ParamPrefix, ParamGroup);
			}
			else
			{
				// pass forward output from previous layer to ensure it makes it to the top
				LayerOutputs.VisualExp = ChildOutputs.VisualExp;
			}
		}
	}


	// merge this layer with the previous sibling
	if (PrevOutputs.ShapeExp && LayerOutputs.ShapeExp)
	{
		// TODO: this is weird, just setup explicit shape merging by layer reference, so the layer structure remains only for visuals
		LayerOutputs.ShapeExp = Generate_MergeShapes(Builder, NodePos, PrevOutputs.ShapeExp, LayerOutputs.ShapeExp,
		                                             Layer->Shape->ShapeMergeOperation, ParamPrefix, ParamGroup);
	}

	if (PrevOutputs.VisualExp)
	{
		if (LayerOutputs.VisualExp)
		{
			LayerOutputs.VisualExp = Generate_MergeVisual(Builder, NodePos, LayerOutputs.VisualExp, PrevOutputs.VisualExp,
			                                              Layer->MergeOperation, ParamPrefix, ParamGroup);
		}
		else
		{
			// pass forward output from previous layer to ensure it makes it to the top
			LayerOutputs.VisualExp = PrevOutputs.VisualExp;
		}
	}

	return LayerOutputs;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_TransformUVs(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                                const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
                                                                const FString& ParamPrefix, const FName& ParamGroup)
{
	// points to the last expression from each operation, since some may be skipped due to optimization
	UMaterialExpression* LastInputExp = InUVsExp;

	// temporary offset used to simplify next node positioning
	FVector2D NodePosOffset = FVector2D::Zero();

	const bool bNoOptimization = Transform.bAnimatable || MGFXMaterial->bAllAnimatable;

	// apply translate
	if (bNoOptimization || !Transform.Location.IsZero())
	{
		NodePos.Y += GridSize * 8;

		UMaterialExpression* TranslateValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Location, ParamPrefix, ParamGroup,
		                                                                   10, Param_LocationX, Param_LocationY);
		NodePos.Y -= GridSize * 8;

		// subtract so that coordinate space matches UMG, positive offset means going right or down
		UMaterialExpressionSubtract* TranslateUVsExp = Builder.Create<UMaterialExpressionSubtract>(NodePos);
		Builder.Connect(LastInputExp, "", TranslateUVsExp, "A");
		Builder.Connect(TranslateValueExp, "", TranslateUVsExp, "B");
		LastInputExp = TranslateUVsExp;

		NodePos.X += GridSize * 15;
	}

	// apply rotate
	if (bNoOptimization || !FMath::IsNearlyZero(Transform.Rotation))
	{
		NodePosOffset = FVector2D(0, GridSize * 8);
		UMaterialExpressionScalarParameter* RotationExp = Builder.CreateScalarParam(
			NodePos + NodePosOffset, FName(ParamPrefix + Param_Rotation), ParamGroup, 20);
		SET_PROP(RotationExp, DefaultValue, Transform.Rotation);

		NodePos.X += GridSize * 15;

		// TODO: perf: allow marking rotation as animatable or not and bake in the conversion when not needed
		// conversion from degrees, not baked in to support animation
		NodePosOffset = FVector2D(0, GridSize * 8);
		UMaterialExpressionDivide* RotateConvExp = Builder.Create<UMaterialExpressionDivide>(NodePos + NodePosOffset);
		Builder.Connect(RotationExp, "", RotateConvExp, "");
		// negative to match UMG coordinate space, positive rotations are clockwise
		SET_PROP_R(RotateConvExp, ConstB, -360.f);

		NodePos.X += GridSize * 15;

		UMaterialExpressionMaterialFunctionCall* RotateUVsExp = Builder.CreateFunction(
			NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Rotate.MF_MGFX_Rotate")));
		Builder.Connect(LastInputExp, "", RotateUVsExp, "UVs");
		Builder.Connect(RotateConvExp, "", RotateUVsExp, "Rotation");
		LastInputExp = RotateUVsExp;

		NodePos.X += GridSize * 15;
	}

	// apply scale
	if (bNoOptimization || Transform.Scale != FVector2f::One())
	{
		NodePos.Y += GridSize * 8;
		UMaterialExpression* ScaleValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Scale,
		                                                               ParamPrefix, ParamGroup, 30, Param_ScaleX, Param_ScaleY);
		NodePos.Y -= GridSize * 8;

		UMaterialExpressionDivide* ScaleUVsExp = Builder.Create<UMaterialExpressionDivide>(NodePos);
		Builder.Connect(LastInputExp, "", ScaleUVsExp, "A");
		Builder.Connect(ScaleValueExp, "", ScaleUVsExp, "B");
		LastInputExp = ScaleUVsExp;

		NodePos.X += GridSize * 15;
	}

	return LastInputExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_Shape(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
                                                         UMaterialExpression* InUVsExp, const FString& ParamPrefix, const FName& ParamGroup)
{
	check(Shape);

	TArray<FMGFXMaterialShapeInput> Inputs = Shape->GetInputs();

	int32 ParamSortPriority = 50;

	// keep track of original Y so it can be restored after generating inputs
	const int32 OrigNodePoseY = NodePos.Y;

	// create shape inputs
	TArray<UMaterialExpression*> InputExps;
	NodePos.Y += GridSize * 8;
	for (const FMGFXMaterialShapeInput& Input : Inputs)
	{
		const FLinearColor Value = Input.Value;

		// TODO: allow user to mark as exposed
		bool bShouldBeParameter = true;

		if (bShouldBeParameter)
		{
			// setup input as scalar params, Vector2's will be represented by 2 scalar params
			UMaterialExpression* InputExp;
			switch (Input.Type)
			{
			default:
			case EMGFXMaterialShapeInputType::Float:
				InputExp = Builder.Create<UMaterialExpressionScalarParameter>(NodePos);
				break;
			case EMGFXMaterialShapeInputType::Vector2:
				InputExp = Generate_Vector2Parameter(Builder, NodePos, FVector2f(Input.Value.R, Input.Value.G),
				                                     ParamPrefix, ParamGroup, ParamSortPriority, Input.Name + TEXT("X"), Input.Name + TEXT("Y"));
				++ParamSortPriority;
				break;
			case EMGFXMaterialShapeInputType::Vector3:
			case EMGFXMaterialShapeInputType::Vector4:
				// its normal to ignore the extra A channel for a Vector3 param
				InputExp = Builder.Create<UMaterialExpressionVectorParameter>(NodePos);
				break;
			}
			check(InputExp);

			InputExps.Add(InputExp);

			// configure parameter
			// Vector2 params will already be configured, and the InputExp is actually an append.
			if (UMaterialExpressionParameter* ParamExp = Cast<UMaterialExpressionParameter>(InputExp))
			{
				// TODO: include Shape name in the prefix, since there may be multiple shapes
				Builder.ConfigureParameter(ParamExp, FName(ParamPrefix + Input.Name), ParamGroup, ParamSortPriority);
				++ParamSortPriority;
			}

			if (auto* ConstExp = Cast<UMaterialExpressionScalarParameter>(InputExp))
			{
				SET_PROP(ConstExp, DefaultValue, Value.R);
			}
			else if (auto* Const2Exp = Cast<UMaterialExpressionVectorParameter>(InputExp))
			{
				SET_PROP(Const2Exp, DefaultValue, Value);
			}

			NodePos.Y += GridSize * 8;
		}
		else
		{
			// setup input as constant expressions
			TSubclassOf<UMaterialExpression> InputExpClass = nullptr;
			switch (Input.Type)
			{
			case EMGFXMaterialShapeInputType::Float:
				InputExpClass = UMaterialExpressionConstant::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector2:
				InputExpClass = UMaterialExpressionConstant2Vector::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector3:
				InputExpClass = UMaterialExpressionConstant3Vector::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector4:
				InputExpClass = UMaterialExpressionConstant4Vector::StaticClass();
				break;
			}
			check(InputExpClass);

			UMaterialExpression* InputExp = Builder.Create(InputExpClass, NodePos);
			check(InputExp);
			InputExps.Add(InputExp);

			if (auto* ConstExp = Cast<UMaterialExpressionConstant>(InputExp))
			{
				SET_PROP(ConstExp, R, Value.R);
			}
			else if (auto* Const2Exp = Cast<UMaterialExpressionConstant2Vector>(InputExp))
			{
				SET_PROP(Const2Exp, R, Value.R);
				SET_PROP(Const2Exp, G, Value.G);
			}
			else if (auto* Const3Exp = Cast<UMaterialExpressionConstant3Vector>(InputExp))
			{
				SET_PROP(Const3Exp, Constant, Value);
			}
			else if (auto* Const4Exp = Cast<UMaterialExpressionConstant4Vector>(InputExp))
			{
				SET_PROP(Const4Exp, Constant, Value);
			}

			NodePos.Y += GridSize * 8;
		}
	}
	check(Inputs.Num() == InputExps.Num());

	NodePos.X += GridSize * 20;
	NodePos.Y = OrigNodePoseY;

	// create shape function
	UMaterialExpressionMaterialFunctionCall* ShapeExp = Builder.Create<UMaterialExpressionMaterialFunctionCall>(NodePos);
	UMaterialFunctionInterface* ShapeFunc = Shape->GetMaterialFunction();
	if (ensure(ShapeFunc))
	{
		SET_PROP(ShapeExp, MaterialFunction, ShapeFunc);
	}

	// connect uvs
	if (InUVsExp)
	{
		Builder.Connect(InUVsExp, "", ShapeExp, "UVs");
	}

	// connect inputs
	for (int32 Idx = 0; Idx < Inputs.Num(); ++Idx)
	{
		const FMGFXMaterialShapeInput& Input = Inputs[Idx];
		UMaterialExpression* InputExp = InputExps[Idx];

		Builder.Connect(InputExp, "", ShapeExp, Input.Name);
	}

	NodePos.X += GridSize * 15;

	return ShapeExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_ShapeVisuals(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
                                                                UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                                const FString& ParamPrefix, const FName& ParamGroup)
{
	// TODO: support multiple visuals
	// generate visuals
	for (const UMGFXMaterialShapeVisual* Visual : Shape->Visuals)
	{
		if (const UMGFXMaterialShapeFill* Fill = Cast<UMGFXMaterialShapeFill>(Visual))
		{
			return Generate_ShapeFill(Builder, NodePos, Fill, ShapeExp, FilterWidthExp, ParamPrefix, ParamGroup);
		}
		else if (const UMGFXMaterialShapeStroke* const Stroke = Cast<UMGFXMaterialShapeStroke>(Visual))
		{
			return Generate_ShapeStroke(Builder, NodePos, Stroke, ShapeExp, FilterWidthExp, ParamPrefix, ParamGroup);
		}
		// TODO: combine visuals, don't just use the last one
	}

	return nullptr;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_MergeVisual(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                               UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXLayerMergeOperation Operation,
                                                               const FString& ParamPrefix, const FName& ParamGroup)
{
	// default to over
	TSoftObjectPtr<UMaterialFunctionInterface> MergeFunc = nullptr;

	switch (Operation)
	{
	case EMGFXLayerMergeOperation::Over:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Over.MF_MGFX_Merge_Over"));
		break;
	case EMGFXLayerMergeOperation::Add:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Add.MF_MGFX_Merge_Add"));
		break;
	case EMGFXLayerMergeOperation::Subtract:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Subtract.MF_MGFX_Merge_Subtract"));
		break;
	case EMGFXLayerMergeOperation::Multiply:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Multiply.MF_MGFX_Merge_Multiply"));
		break;
	case EMGFXLayerMergeOperation::In:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_In.MF_MGFX_Merge_In"));
		break;
	case EMGFXLayerMergeOperation::Out:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Out.MF_MGFX_Merge_Out"));
		break;
	case EMGFXLayerMergeOperation::Mask:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Mask.MF_MGFX_Merge_Mask"));
		break;
	case EMGFXLayerMergeOperation::Stencil:
		MergeFunc = TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Merge_Stencil.MF_MGFX_Merge_Stencil"));
		break;
	default: ;
	}

	if (MergeFunc.IsNull())
	{
		// no valid merge function
		return AExp;
	}

	UMaterialExpressionMaterialFunctionCall* MergeExp = Builder.CreateFunction(NodePos, MergeFunc);
	Builder.Connect(AExp, "", MergeExp, "A");
	Builder.Connect(BExp, "", MergeExp, "B");

	NodePos.X += GridSize * 15;

	return MergeExp;
}


UMaterialExpression* FMGFXMaterialEditor::Generate_MergeShapes(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                               UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXShapeMergeOperation Operation,
                                                               const FString& ParamPrefix, const FName& ParamGroup)
{
	if (Operation == EMGFXShapeMergeOperation::None)
	{
		return BExp;
	}

	return nullptr;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_Vector2Parameter(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, FVector2f DefaultValue,
                                                                    const FString& ParamPrefix, const FName& ParamGroup, int32 BaseSortPriority,
                                                                    const FString& ParamNameX, const FString& ParamNameY)
{
	UMaterialExpressionScalarParameter* XExp = Builder.CreateScalarParam(
		NodePos, FName(ParamPrefix + ParamNameX), ParamGroup, BaseSortPriority);
	SET_PROP(XExp, DefaultValue, DefaultValue.X);

	UMaterialExpressionScalarParameter* YExp = Builder.CreateScalarParam(
		NodePos + FVector2D(0, GridSize * 6), FName(ParamPrefix + ParamNameY), ParamGroup, BaseSortPriority + 1);
	SET_PROP(YExp, DefaultValue, DefaultValue.Y);

	NodePos.X += GridSize * 15;

	// append
	UMaterialExpressionAppendVector* AppendExp = Builder.CreateAppend(NodePos, XExp, YExp);

	NodePos.X += GridSize * 15;

	return AppendExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_ShapeFill(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                             const UMGFXMaterialShapeFill* Fill, UMaterialExpression* ShapeExp,
                                                             UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                             const FString& ParamPrefix, const FName& ParamGroup)
{
	// add reused filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthUsageExp = nullptr;
	if (!Fill->bComputeFilterWidth)
	{
		FilterWidthUsageExp = Builder.CreateNamedRerouteUsage(NodePos + FVector2D(0, GridSize * 8), FilterWidthExp);
	}

	// optionally enable filter bias
	UMaterialExpressionStaticBool* EnableBiasExp = nullptr;
	if (Fill->bEnableFilterBias)
	{
		EnableBiasExp = Builder.Create<UMaterialExpressionStaticBool>(NodePos + FVector2D(0, GridSize * 16));
		SET_PROP_R(EnableBiasExp, Value, 1);
	}

	NodePos.X += GridSize * 15;

	// add fill func
	UMaterialExpressionMaterialFunctionCall* FillExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Fill.MF_MGFX_Fill")));
	Builder.Connect(ShapeExp, "SDF", FillExp, "SDF");
	if (FilterWidthUsageExp)
	{
		Builder.Connect(FilterWidthUsageExp, "", FillExp, "FilterWidth");
	}
	if (EnableBiasExp)
	{
		Builder.Connect(EnableBiasExp, "", FillExp, "EnableFilterBias");
	}

	NodePos.X += GridSize * 15;

	// add color param
	UMaterialExpressionVectorParameter* ColorExp = Builder.Create<UMaterialExpressionVectorParameter>(NodePos + FVector2D(0, GridSize * 8));
	Builder.ConfigureParameter(ColorExp, FName(ParamPrefix + "Color"), ParamGroup, 40);
	SET_PROP_R(ColorExp, DefaultValue, Fill->GetColor());

	NodePos.X += GridSize * 15;

	// mutiply by color, and append to RGBA
	UMaterialExpressionMaterialFunctionCall* TintExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Tint.MF_MGFX_Tint")));
	Builder.Connect(FillExp, "", TintExp, "In");
	Builder.Connect(ColorExp, "", TintExp, "RGB");
	Builder.Connect(ColorExp, "A", TintExp, "A");

	NodePos.X += GridSize * 15;

	return TintExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_ShapeStroke(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                               const UMGFXMaterialShapeStroke* Stroke, UMaterialExpression* ShapeExp,
                                                               UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                               const FString& ParamPrefix, const FName& ParamGroup)
{
	// add stroke width input
	UMaterialExpressionScalarParameter* StrokeWidthExp = Builder.CreateScalarParam(
		NodePos + FVector2D(0, GridSize * 8), FName(ParamPrefix + "StrokeWidth"), ParamGroup, 40);
	SET_PROP(StrokeWidthExp, DefaultValue, Stroke->StrokeWidth);

	// add reused filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthUsageExp = nullptr;
	if (!Stroke->bComputeFilterWidth)
	{
		FilterWidthUsageExp = Builder.CreateNamedRerouteUsage(NodePos + FVector2D(0, GridSize * 14), FilterWidthExp);
	}

	NodePos.X += GridSize * 15;

	// add stroke func
	UMaterialExpressionMaterialFunctionCall* StrokeExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Stroke.MF_MGFX_Stroke")));
	Builder.Connect(ShapeExp, "SDF", StrokeExp, "SDF");
	Builder.Connect(StrokeWidthExp, "", StrokeExp, "StrokeWidth");
	if (FilterWidthUsageExp)
	{
		Builder.Connect(FilterWidthUsageExp, "", StrokeExp, "FilterWidth");
	}

	NodePos.X += GridSize * 15;

	// add color param
	UMaterialExpressionVectorParameter* ColorExp = Builder.Create<UMaterialExpressionVectorParameter>(NodePos + FVector2D(0, GridSize * 8));
	Builder.ConfigureParameter(ColorExp, FName(ParamPrefix + "Color"), ParamGroup, 40);
	SET_PROP_R(ColorExp, DefaultValue, Stroke->GetColor());

	NodePos.X += GridSize * 15;

	// mutiply by color, and append to RGBA
	UMaterialExpressionMaterialFunctionCall* TintExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Tint.MF_MGFX_Tint")));
	Builder.Connect(StrokeExp, "", TintExp, "In");
	Builder.Connect(ColorExp, "", TintExp, "RGB");
	Builder.Connect(ColorExp, "A", TintExp, "A");

	NodePos.X += GridSize * 15;

	return TintExp;
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
	// TODO
}

bool FMGFXMaterialEditor::CanCopySelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::CutSelectedLayers()
{
	// TODO

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanCutSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::PasteLayers()
{
	// TODO

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanPasteLayers()
{
	return true;
}

void FMGFXMaterialEditor::DuplicateSelectedLayers()
{
	// TODO

	OnLayersChanged();
}

bool FMGFXMaterialEditor::CanDuplicateSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

FString FMGFXMaterialEditor::MakeUniqueLayerName(const FString& Name, const UMGFXMaterial* InMaterial)
{
	if (!InMaterial)
	{
		return Name;
	}

	TArray<TObjectPtr<UMGFXMaterialLayer>> AllLayers;
	InMaterial->GetAllLayers(AllLayers);

	FString NewName = Name;

	int32 Number = 1;
	while (AllLayers.FindByPredicate([NewName](const TObjectPtr<UMGFXMaterialLayer> Layer)
	{
		return Layer->Name.Equals(NewName);
	}))
	{
		NewName = Name + FString::FromInt(Number);
		++Number;
	}

	return NewName;
}


#undef LOCTEXT_NAMESPACE
