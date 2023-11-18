// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "IMaterialEditor.h"
#include "MaterialEditorActions.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MGFXMaterialLayer.h"
#include "MGFXPropertyMacros.h"
#include "ObjectEditorUtils.h"
#include "SMGFXMaterialEditorCanvas.h"
#include "SMGFXMaterialEditorLayers.h"
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
const FName FMGFXMaterialEditor::Reroute_FilterWidth(TEXT("FilterWidth"));
const FName FMGFXMaterialEditor::Reroute_LayersOutput(TEXT("LayersOutput"));
const int32 FMGFXMaterialEditor::GridSize(16);


FMGFXMaterialEditor::FMGFXMaterialEditor()
	: NodePosBaselineLeft(GridSize * -64),
	  SDFRerouteColor(FLinearColor(0.f, 0.f, 0.f)),
	  RGBARerouteColor(FLinearColor(0.22f, .09f, 0.55f))
{
}

void FMGFXMaterialEditor::InitMGFXMaterialEditor(const EToolkitMode::Type Mode,
                                                 const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                 UMGFXMaterial* InMGFXMaterial)
{
	check(InMGFXMaterial);

	OriginalMGFXMaterial = InMGFXMaterial;

	FMGFXMaterialEditorCommands::Register();
	BindCommands();

	// initialize details view
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FMGFXMaterialEditor::IsDetailsPropertyVisible));
	DetailsView->SetIsCustomRowVisibleDelegate(FIsCustomRowVisible::CreateSP(this, &FMGFXMaterialEditor::IsDetailsRowVisible));
	DetailsView->SetObject(OriginalMGFXMaterial);

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
	                OriginalMGFXMaterial);

	ExtendToolbar();
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
	return OriginalMGFXMaterial ? OriginalMGFXMaterial->GeneratedMaterial : nullptr;
}

IMaterialEditor* FMGFXMaterialEditor::GetOrOpenMaterialEditor(UMaterial* Material) const
{
	check(Material);

	// find or open the material editor
	IAssetEditorInstance* OpenEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Material, true);
	IMaterialEditor* MaterialEditor = static_cast<IMaterialEditor*>(OpenEditor);
	if (!MaterialEditor)
	{
		// try to open the editor
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Material))
		{
			return nullptr;
		}

		OpenEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Material, true);
		MaterialEditor = static_cast<IMaterialEditor*>(OpenEditor);

		if (!MaterialEditor)
		{
			return nullptr;
		}
	}

	return MaterialEditor;
}

void FMGFXMaterialEditor::RegenerateMaterial()
{
	UMaterial* Material = GetGeneratedMaterial();
	if (!Material)
	{
		// create one?
		return;
	}

	// TODO: close material editor, or find a way to refresh it, could just copy (manual duplicate) to the preview material?

	const FScopedTransaction Transaction(LOCTEXT("RegenerateMaterial", "MGFX Material Editor: Regenerate Material"));

	FMGFXMaterialBuilder Builder(Material);

	Generate_DeleteAllNodes(Builder);
	Generate_AddWarningComment(Builder);
	Generate_AddUVsBoilerplate(Builder);
	Generate_Layers(Builder);

	// connect layer output to final color
	UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(FVector2D(GridSize * -16, 0), Reroute_LayersOutput);
	Builder.ConnectProperty(OutputUsageExp, "", MP_EmissiveColor);

	Builder.RecompileMaterial();

	// TODO: use events to keep this up to date
	// update canvas artboard
	MGFXMaterialEditorCanvas->UpdateArtboardSize();
}

FVector2D FMGFXMaterialEditor::GetCanvasSize() const
{
	return FVector2D(OriginalMGFXMaterial->BaseCanvasSize);
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

	if (SelectedLayers.Num() == 1)
	{
		DetailsView->SetObject(SelectedLayers[0]);
	}
	else
	{
		// default to inspecting material when nothing or multiple things are selected
		DetailsView->SetObject(OriginalMGFXMaterial);
	}

	OnLayerSelectionChangedEvent.Broadcast(SelectedLayers);
}

void FMGFXMaterialEditor::ClearSelectedLayers()
{
	SetSelectedLayers(TArray<UMGFXMaterialLayer*>());
}

void FMGFXMaterialEditor::OnLayerSelectionChanged(UMGFXMaterialLayer* Layer)
{
	check(DetailsView.IsValid());

	if (Layer)
	{
		SetSelectedLayers({Layer});
	}
	else
	{
		ClearSelectedLayers();
	}
}

bool FMGFXMaterialEditor::IsDetailsPropertyVisible(const FPropertyAndParent& PropertyAndParent)
{
	const TArray<FName> CategoriesToHide = {};

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
	const TArray<FName> CategoriesToHide = {};

	if (CategoriesToHide.Contains(InParentName))
	{
		return false;
	}
	return true;
}

void FMGFXMaterialEditor::NotifyPreChange(FProperty* PropertyAboutToChange)
{
}

void FMGFXMaterialEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	// TODO: filter properties that cause regenerate
	RegenerateMaterial();
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

void FMGFXMaterialEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolBarBuilder)
		{
			ToolBarBuilder.BeginSection("Actions");
			{
				ToolBarBuilder.AddToolBarButton(FMGFXMaterialEditorCommands::Get().RegenerateMaterial);
			}
		}
	};

	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
	);

	AddToolbarExtender(Extender);
}


TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Canvas(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SAssignNew(MGFXMaterialEditorCanvas, SMGFXMaterialEditorCanvas)
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
	];
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		DetailsView.ToSharedRef()
	];
}

void FMGFXMaterialEditor::Generate_DeleteAllNodes(FMGFXMaterialBuilder& Builder)
{
	Builder.Material->GetExpressionCollection().Empty();
}

void FMGFXMaterialEditor::Generate_AddWarningComment(FMGFXMaterialBuilder& Builder)
{
	const FLinearColor CommentColor = FLinearColor(0.06f, 0.02f, 0.02f);
	const FString Text = FString::Printf(TEXT("Generated by %s\nDo not edit manually"), *GetNameSafe(OriginalMGFXMaterial));

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
	auto* CanvasAppendExp = Generate_Vector2Parameter(Builder, NodePos, OriginalMGFXMaterial->BaseCanvasSize,
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

	if (OriginalMGFXMaterial->bComputeFilterWidth)
	{
		UMaterialExpressionMaterialFunctionCall* FilterWidthFuncExp = Builder.CreateFunction(
			NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_FilterWidth.MF_MGFX_FilterWidth")));
		Builder.Connect(OutputRerouteExp, "", FilterWidthFuncExp, "");

		NodePos.X += GridSize * 15;

		// apply filter width scale if not 1x
		if (!FMath::IsNearlyEqual(OriginalMGFXMaterial->FilterWidthScale, 1.f))
		{
			UMaterialExpressionMultiply* FilterWidthScaleExp = Builder.Create<UMaterialExpressionMultiply>(NodePos);
			Builder.Connect(FilterWidthFuncExp, "", FilterWidthScaleExp, "");
			SET_PROP(FilterWidthScaleExp, ConstB, OriginalMGFXMaterial->FilterWidthScale);

			NodePos.X += GridSize * 15;

			FilterWidthExp = FilterWidthScaleExp;
		}
		else
		{
			FilterWidthExp = FilterWidthFuncExp;
		}
	}
	else
	{
		UMaterialExpressionConstant* FilterWidthConstExp = Builder.Create<UMaterialExpressionConstant>(NodePos);
		SET_PROP(FilterWidthConstExp, R, OriginalMGFXMaterial->FilterWidthScale);

		NodePos.X += GridSize * 15;

		FilterWidthExp = FilterWidthConstExp;
	}

	// add filter width reroute
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthRerouteExp = Builder.CreateNamedReroute(NodePos, Reroute_FilterWidth);
	Builder.Connect(FilterWidthExp, "", FilterWidthRerouteExp, "");
}

void FMGFXMaterialEditor::Generate_Layers(FMGFXMaterialBuilder& Builder)
{
	FVector2D NodePos(NodePosBaselineLeft, GridSize * 80);

	// start with background color
	UMaterialExpressionConstant4Vector* BGColorExp = Builder.Create<UMaterialExpressionConstant4Vector>(NodePos);
	SET_PROP(BGColorExp, Constant, FLinearColor::Black);

	NodePos.X += GridSize * 15;

	const FMGFXMaterialLayerOutputs BGOutputs(nullptr, nullptr, BGColorExp);

	// create base UVs
	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = Builder.FindNamedReroute(Reroute_CanvasUVs);
	check(CanvasUVsDeclaration);

	// generate all layers recursively
	check(OriginalMGFXMaterial->RootLayer);
	const FMGFXMaterialLayerOutputs TopLayerOutputs = Generate_Layer(Builder, NodePos, OriginalMGFXMaterial->RootLayer, CanvasUVsDeclaration, BGOutputs);

	NodePos.X += GridSize * 15;

	// connect to layers output reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = Builder.CreateNamedReroute(NodePos, Reroute_LayersOutput, RGBARerouteColor);
	Builder.Connect(TopLayerOutputs.VisualExp, "", OutputRerouteExp, "");
}

FMGFXMaterialLayerOutputs FMGFXMaterialEditor::Generate_Layer(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialLayer* Layer,
                                                              UMaterialExpressionNamedRerouteDeclaration* InputUVsReroute,
                                                              const FMGFXMaterialLayerOutputs& PrevOutputs)
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
	UMaterialExpressionNamedRerouteUsage* ParentUVsExp = Builder.CreateNamedRerouteUsage(NodePos, InputUVsReroute);

	NodePos.X += GridSize * 15;

	// apply layer transform (if needed), and store as a reroute for children (if needed)
	const bool bCreateReroute = Layer->HasChildren();
	UMaterialExpression* UVsExp = Generate_TransformUVs(Builder, NodePos, Layer->Transform, ParentUVsExp, ParamPrefix, ParamGroup, bCreateReroute);

	if (bCreateReroute)
	{
		LayerOutputs.UVsExp = Cast<UMaterialExpressionNamedRerouteDeclaration>(UVsExp);
	}


	// generate children layers bottom to top
	FMGFXMaterialLayerOutputs ChildOutputs = LayerOutputs;
	for (int32 Idx = Layer->NumChildren() - 1; Idx >= 0; --Idx)
	{
		const UMGFXMaterialLayer* ChildLayer = Layer->GetChild(Idx);

		check(LayerOutputs.UVsExp);
		ChildOutputs = Generate_Layer(Builder, NodePos, ChildLayer, LayerOutputs.UVsExp, ChildOutputs);
	}


	// generate shape for this layer
	if (Layer->Shape)
	{
		// the shape, with an SDF output
		LayerOutputs.ShapeExp = Generate_Shape(Builder, NodePos, Layer->Shape, UVsExp, ParamPrefix, ParamGroup);

		// the shape visuals, including all fills and strokes
		LayerOutputs.VisualExp = Generate_ShapeVisuals(Builder, NodePos, Layer->Shape, LayerOutputs.ShapeExp, ParamPrefix, ParamGroup);
	}

	// merge this layer with its children
	if (Layer->HasChildren())
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
                                                                const FString& ParamPrefix, const FName& ParamGroup, bool bCreateReroute)
{
	// points to the last expression from each operation, since some may be skipped due to optimization
	UMaterialExpression* LastInputExp = InUVsExp;

	// temporary offset used to simplify next node positioning
	FVector2D NodePosOffset = FVector2D::Zero();

	const bool bNoOptimization = Transform.bAnimatable || OriginalMGFXMaterial->bAllAnimatable;

	// apply translate
	if (bNoOptimization || !Transform.Location.IsZero())
	{
		NodePos.Y += GridSize * 8;

		UMaterialExpression* TranslateValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Location, ParamPrefix, ParamGroup,
		                                                                   10, "TranslateX", "TranslateY");
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
			NodePos + NodePosOffset, FName(ParamPrefix + "Rotation"), ParamGroup, 20);
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
		UMaterialExpression* ScaleValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Scale, ParamPrefix, ParamGroup, 30, "ScaleX", "ScaleY");
		NodePos.Y -= GridSize * 8;

		UMaterialExpressionDivide* ScaleUVsExp = Builder.Create<UMaterialExpressionDivide>(NodePos);
		Builder.Connect(LastInputExp, "", ScaleUVsExp, "A");
		Builder.Connect(ScaleValueExp, "", ScaleUVsExp, "B");
		LastInputExp = ScaleUVsExp;

		NodePos.X += GridSize * 15;
	}

	if (bCreateReroute)
	{
		// output to named reroute
		UMaterialExpressionNamedRerouteDeclaration* UVsRerouteExp = Builder.CreateNamedReroute(NodePos, FName(ParamPrefix + "UVs"));
		Builder.Connect(LastInputExp, "", UVsRerouteExp, "");
		LastInputExp = UVsRerouteExp;

		NodePos.X += GridSize * 15;
	}

	return LastInputExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_Shape(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
                                                         UMaterialExpression* InUVsExp, const FString& ParamPrefix, const FName& ParamGroup)
{
	check(Shape);

	TArray<FMGFXMaterialShapeInput> Inputs = Shape->GetInputs();

	// create shape inputs
	TArray<UMaterialExpression*> InputExps;
	FVector2D NodePosOffset = FVector2D(0.f, GridSize * 8);
	for (const FMGFXMaterialShapeInput& Input : Inputs)
	{
		// determine expression class by input type
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

		const FLinearColor Value = Input.Value;

		UMaterialExpression* InputExp = Builder.Create(InputExpClass, NodePos + NodePosOffset);
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

		NodePosOffset.Y += GridSize * 8;
	}
	check(Inputs.Num() == InputExps.Num());

	NodePos.X += GridSize * 20;

	// create shape function
	UMaterialExpressionMaterialFunctionCall* ShapeExp = Builder.Create<UMaterialExpressionMaterialFunctionCall>(NodePos);
	UMaterialFunctionInterface* ShapeFunc = Shape->GetMaterialFunction();
	check(ShapeFunc);
	SET_PROP(ShapeExp, MaterialFunction, ShapeFunc);
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

UMaterialExpression* FMGFXMaterialEditor::Generate_ShapeVisuals(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                                const UMGFXMaterialShape* Shape, UMaterialExpression* ShapeExp,
                                                                const FString& ParamPrefix, const FName& ParamGroup)
{
	// TODO: support multiple visuals
	// generate visuals
	for (const UMGFXMaterialShapeVisual* Visual : Shape->Visuals)
	{
		if (const UMGFXMaterialShapeFill* Fill = Cast<UMGFXMaterialShapeFill>(Visual))
		{
			return Generate_ShapeFill(Builder, NodePos, Fill, ShapeExp, ParamPrefix, ParamGroup);
		}
		else if (const UMGFXMaterialShapeStroke* const Stroke = Cast<UMGFXMaterialShapeStroke>(Visual))
		{
			return Generate_ShapeStroke(Builder, NodePos, Stroke, ShapeExp, ParamPrefix, ParamGroup);
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
                                                             const FString& ParamPrefix, const FName& ParamGroup)
{
	// add filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthExp = Builder.CreateNamedRerouteUsage(NodePos + FVector2D(0, GridSize * 8), Reroute_FilterWidth);

	NodePos.X += GridSize * 15;

	// add fill func
	UMaterialExpressionMaterialFunctionCall* FillExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Fill.MF_MGFX_Fill")));
	Builder.Connect(ShapeExp, "SDF", FillExp, "SDF");
	Builder.Connect(FilterWidthExp, "", FillExp, "FilterWidth");

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
                                                               const FString& ParamPrefix, const FName& ParamGroup)
{
	// add stroke width input
	UMaterialExpressionScalarParameter* StrokeWidthExp = Builder.CreateScalarParam(
		NodePos + FVector2D(0, GridSize * 8), FName(ParamPrefix + "StrokeWidth"), ParamGroup, 40);
	SET_PROP(StrokeWidthExp, DefaultValue, Stroke->StrokeWidth);

	// add filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthExp = Builder.CreateNamedRerouteUsage(NodePos + FVector2D(0, GridSize * 14), Reroute_FilterWidth);

	NodePos.X += GridSize * 15;

	// add stroke func
	UMaterialExpressionMaterialFunctionCall* StrokeExp = Builder.CreateFunction(
		NodePos, TSoftObjectPtr<UMaterialFunctionInterface>(FString("/MGFX/MaterialFunctions/MF_MGFX_Stroke.MF_MGFX_Stroke")));
	Builder.Connect(ShapeExp, "SDF", StrokeExp, "SDF");
	Builder.Connect(StrokeWidthExp, "", StrokeExp, "StrokeWidth");
	Builder.Connect(FilterWidthExp, "", StrokeExp, "FilterWidth");

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

void FMGFXMaterialEditor::ApplyMaterial(IMaterialEditor* MaterialEditor)
{
	const TSharedRef<FUICommandList> MaterialEditorCommands = MaterialEditor->GetToolkitCommands();
	const FUIAction* ApplyAction = MaterialEditorCommands->GetActionForCommand(FMaterialEditorCommands::Get().Apply);
	if (ApplyAction)
	{
		ApplyAction->Execute();
	}
}

void FMGFXMaterialEditor::DeleteSelectedLayers()
{
	TArray<UMGFXMaterialLayer*> LayersToDelete = SelectedLayers.FilterByPredicate([](const UMGFXMaterialLayer* Layer)
	{
		return Layer->GetParent() != nullptr;
	});

	ClearSelectedLayers();

	{
		FScopedTransaction Transaction(LOCTEXT("DeleteLayer", "Delete Layer"));
		OriginalMGFXMaterial->Modify();

		for (UMGFXMaterialLayer* Layer : LayersToDelete)
		{
			// don't delete the root layer
			if (!Layer->GetParent())
			{
				continue;
			}

			Layer->GetParent()->RemoveChild(Layer);
		}
	}

	OnLayersChangedEvent.Broadcast();
}

bool FMGFXMaterialEditor::CanDeleteSelectedLayers()
{
	const TArray<UMGFXMaterialLayer*> LayersToDelete = SelectedLayers.FilterByPredicate([](const UMGFXMaterialLayer* Layer)
	{
		return Layer->GetParent() != nullptr;
	});
	return !LayersToDelete.IsEmpty();
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

	OnLayersChangedEvent.Broadcast();
}

bool FMGFXMaterialEditor::CanCutSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}

void FMGFXMaterialEditor::PasteLayers()
{
	// TODO

	OnLayersChangedEvent.Broadcast();
}

bool FMGFXMaterialEditor::CanPasteLayers()
{
	return true;
}

void FMGFXMaterialEditor::DuplicateSelectedLayers()
{
	// TODO

	OnLayersChangedEvent.Broadcast();
}

bool FMGFXMaterialEditor::CanDuplicateSelectedLayers()
{
	return !SelectedLayers.IsEmpty();
}


#undef LOCTEXT_NAMESPACE
