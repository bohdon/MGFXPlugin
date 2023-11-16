// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "IMaterialEditor.h"
#include "MaterialEditorActions.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MGFXPropertyMacros.h"
#include "Artboard/SArtboardPanel.h"
#include "MaterialGraph/MaterialGraph.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Shapes/MGFXMaterialShapeVisual.h"
#include "UObject/PropertyAccessUtil.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


constexpr auto ReadOnlyFlags = PropertyAccessUtil::EditorReadOnlyFlags;
constexpr auto NotifyMode = EPropertyAccessChangeNotifyMode::Default;

const FName FMGFXMaterialEditor::DetailsTabId(TEXT("MGFXMaterialEditorDetailsTab"));
const FName FMGFXMaterialEditor::CanvasTabId(TEXT("MGFXMaterialEditorCanvasTab"));
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

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MGFXMaterialEditor_v0.1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.9f)
					->AddTab(CanvasTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
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


	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Details))
	            .SetDisplayName(LOCTEXT("DetailsTabTitle", "Details"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

UMaterial* FMGFXMaterialEditor::GetGeneratedMaterial() const
{
	return OriginalMGFXMaterial ? OriginalMGFXMaterial->GeneratedMaterial : nullptr;
}

void FMGFXMaterialEditor::RegenerateMaterial()
{
	UMaterial* OriginalMaterial = GetGeneratedMaterial();
	if (!OriginalMaterial)
	{
		return;
	}

	// find or open the material editor
	IAssetEditorInstance* OpenEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(OriginalMaterial, true);
	IMaterialEditor* MaterialEditor = static_cast<IMaterialEditor*>(OpenEditor);
	if (!MaterialEditor)
	{
		// try to open the editor
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(OriginalMaterial))
		{
			return;
		}

		OpenEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(OriginalMaterial, true);
		MaterialEditor = static_cast<IMaterialEditor*>(OpenEditor);

		if (!MaterialEditor)
		{
			return;
		}
	}

	// the preview material has the graph we need to operate on
	const UMaterial* EditorMaterial = Cast<UMaterial>(MaterialEditor->GetMaterialInterface());
	UMaterialGraph* MaterialGraph = EditorMaterial->MaterialGraph;

	FMGFXMaterialBuilder Builder(MaterialEditor, MaterialGraph);

	Generate_DeleteAllNodes(Builder);
	Generate_AddWarningComment(Builder);
	Generate_AddUVsBoilerplate(Builder);
	Generate_Layers(Builder);

	// connect layer output to final color
	UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(FVector2D(GridSize * -16, 0), Reroute_LayersOutput);
	Builder.ConnectProperty(OutputUsageExp, "", MP_EmissiveColor);

	MaterialGraph->LinkGraphNodesFromMaterial();
	MaterialEditor->UpdateMaterialAfterGraphChange();

	ApplyMaterial(MaterialEditor);

	// update preview image
	if (ArtboardPanel.IsValid())
	{
		if (SArtboardPanel::FSlot* Slot = ArtboardPanel->GetWidgetSlot(PreviewImage))
		{
			Slot->SetSize(OriginalMGFXMaterial->BaseCanvasSize);
		}
	}
}

FVector2D FMGFXMaterialEditor::GetCanvasSize() const
{
	return FVector2D(OriginalMGFXMaterial->BaseCanvasSize);
}

void FMGFXMaterialEditor::BindCommands()
{
	const FMGFXMaterialEditorCommands& Commands = FMGFXMaterialEditorCommands::Get();
	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(
		Commands.RegenerateMaterial,
		FExecuteAction::CreateSP(this, &FMGFXMaterialEditor::RegenerateMaterial));
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
	check(OriginalMGFXMaterial);

	// TODO: ensure a generated material is assigned? or update it on change
	PreviewImageBrush.SetResourceObject(OriginalMGFXMaterial->GeneratedMaterial);

	return SNew(SDockTab)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(4.f)
		[
			SAssignNew(ArtboardPanel, SArtboardPanel)
			.ArtboardSize(this, &FMGFXMaterialEditor::GetCanvasSize)
			.BackgroundBrush(FSlateColorBrush(FLinearColor(0.002f, 0.002f, 0.002f)))
			.Clipping(EWidgetClipping::ClipToBounds)
			+ SArtboardPanel::Slot()
			  .Position(FVector2D::ZeroVector)
			  .Size(OriginalMGFXMaterial->BaseCanvasSize)
			[
				SAssignNew(PreviewImage, SImage)
				.Image(&PreviewImageBrush)
			]
		]
	];
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	const TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(OriginalMGFXMaterial);

	return SNew(SDockTab)
	[
		DetailsView
	];
}

void FMGFXMaterialEditor::Generate_DeleteAllNodes(FMGFXMaterialBuilder& Builder)
{
	auto NodesToDelete = Builder.MaterialGraph->Nodes.FilterByPredicate([](const UEdGraphNode* Node)
	{
		return Node->CanUserDeleteNode();
	});
	Builder.MaterialEditor->DeleteNodes(NodesToDelete);
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

	// generate all layers recursively
	check(OriginalMGFXMaterial->RootLayer);
	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = Builder.FindNamedReroute(Reroute_CanvasUVs);
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
	const FName ParamGroup = FName(FString::Printf(TEXT("[%d] %s"), Layer->Index, *LayerName));

	// reset baseline and move to new line
	NodePos.X = NodePosBaselineLeft;
	NodePos.Y += GridSize * 40;


	// generate transform uvs
	UMaterialExpressionNamedRerouteUsage* ParentUVsExp = Builder.CreateNamedRerouteUsage(NodePos, InputUVsReroute);

	NodePos.X += GridSize * 15;

	// apply layer transform (if needed), and store as a reroute for children (if needed)
	const bool bCreateReroute = !Layer->Children.IsEmpty();
	UMaterialExpression* UVsExp = Generate_TransformUVs(Builder, NodePos, Layer->Transform, ParentUVsExp, ParamPrefix, ParamGroup, bCreateReroute);

	if (bCreateReroute)
	{
		LayerOutputs.UVsExp = Cast<UMaterialExpressionNamedRerouteDeclaration>(UVsExp);
	}


	// generate children layers bottom to top
	FMGFXMaterialLayerOutputs ChildOutputs = LayerOutputs;
	for (int32 Idx = Layer->Children.Num() - 1; Idx >= 0; --Idx)
	{
		check(LayerOutputs.UVsExp);
		const UMGFXMaterialLayer* ChildLayer = Layer->Children[Idx];
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
	if (Layer->Children.Num() > 0)
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

		UMaterialExpression* InputExp = Builder.MaterialEditor->CreateNewMaterialExpression(InputExpClass, NodePos + NodePosOffset, false, false);
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


#undef LOCTEXT_NAMESPACE
