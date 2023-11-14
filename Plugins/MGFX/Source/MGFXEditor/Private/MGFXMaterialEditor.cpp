// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "IMaterialEditor.h"
#include "MaterialEditingLibrary.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "UObject/PropertyAccessUtil.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


/** Set the property of an object, ensuring property change callbacks are triggered. */
#define SET_PROPERTY(Object, Class, PropName, Value) \
	{ \
		const FProperty* Prop = PropertyAccessUtil::FindPropertyByName( \
			GET_MEMBER_NAME_CHECKED(Class, PropName), Class::StaticClass()); \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, &Value, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

/** Set the property of an object, accepting an r-value. */
#define SET_PROPERTY_R(Object, Class, PropName, RValue) \
	{ \
		const FProperty* Prop = PropertyAccessUtil::FindPropertyByName( \
			GET_MEMBER_NAME_CHECKED(Class, PropName), Class::StaticClass()); \
		auto Value = RValue; \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, &Value, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

#define SET_PROPERTY_PTR(Object, Class, PropName, PtrValue) \
	{ \
		const FProperty* Prop = PropertyAccessUtil::FindPropertyByName( \
			GET_MEMBER_NAME_CHECKED(Class, PropName), Class::StaticClass()); \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, PtrValue, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

/** Create a new material expression with default settings. */
#define MNew(Class, NodePos) \
	Cast<Class>(MaterialEditor->CreateNewMaterialExpression( \
		Class::StaticClass(), \
		NodePos, false, false))

/** Connect two material expressions. */
#define MConnect(From, FromPin, To, ToPin) \
	UMaterialEditingLibrary::ConnectMaterialExpressions(From, FString(FromPin), To, FString(ToPin))

/** Connect a material expression to a material output property. */
#define MConnectProp(From, FromPin, Property) \
	UMaterialEditingLibrary::ConnectMaterialProperty(From, FString(FromPin), Property)


constexpr auto ReadOnlyFlags = PropertyAccessUtil::EditorReadOnlyFlags;
constexpr auto NotifyMode = EPropertyAccessChangeNotifyMode::Default;

const FName FMGFXMaterialEditor::DetailsTabId(TEXT("MGFXMaterialEditorDetailsTab"));
const FName FMGFXMaterialEditor::CanvasTabId(TEXT("MGFXMaterialEditorCanvasTab"));
const FName FMGFXMaterialEditor::Reroute_CanvasUVs(TEXT("CanvasUVs"));
const FName FMGFXMaterialEditor::Reroute_ShapesOutput(TEXT("ShapesOutput"));
const int32 FMGFXMaterialEditor::GridSize(16);


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

	InTabManager->RegisterTabSpawner(CanvasTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Canvas))
	            .SetDisplayName(LOCTEXT("CanvasTabTitle", "Canvas"));


	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Details))
	            .SetDisplayName(LOCTEXT("DetailsTabTitle", "Details"));
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

	Generate_DeleteAllNodes(MaterialEditor, MaterialGraph);
	Generate_AddWarningComment(MaterialEditor, MaterialGraph);
	Generate_AddUVsBoilerplate(MaterialEditor, MaterialGraph);
	Generate_Shapes(MaterialEditor, MaterialGraph);

	// connect shapes output to opacity
	UMaterialExpressionNamedRerouteDeclaration* ShapesOutputRerouteExp = FindNamedReroute(MaterialGraph, Reroute_ShapesOutput);
	check(ShapesOutputRerouteExp);

	UMaterialExpressionNamedRerouteUsage* OutputUsageExp = MNew(UMaterialExpressionNamedRerouteUsage, FVector2D(GridSize * -16, 0));
	OutputUsageExp->Declaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(ShapesOutputRerouteExp);
	MConnectProp(OutputUsageExp, "", MP_EmissiveColor);

	MaterialGraph->LinkGraphNodesFromMaterial();
	MaterialEditor->UpdateMaterialAfterGraphChange();
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
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
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

void FMGFXMaterialEditor::Generate_DeleteAllNodes(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph)
{
	auto NodesToDelete = MaterialGraph->Nodes.FilterByPredicate([](const UEdGraphNode* Node)
	{
		return Node->CanUserDeleteNode();
	});
	MaterialEditor->DeleteNodes(NodesToDelete);
}

void FMGFXMaterialEditor::Generate_AddWarningComment(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph)
{
	const FLinearColor CommentColor = FLinearColor(0.06f, 0.02f, 0.02f);
	const FString Text = FString::Printf(TEXT("Generated by %s\nDo not edit manually"), *GetNameSafe(OriginalMGFXMaterial));

	FVector2D NodePos(GridSize * -20, GridSize * -10);
	UMaterialExpressionComment* CommentExp = MaterialEditor->CreateNewMaterialExpressionComment(NodePos);
	SET_PROPERTY(CommentExp, UMaterialExpressionComment, CommentColor, CommentColor);
	SET_PROPERTY(CommentExp, UMaterialExpressionComment, Text, Text);
}

void FMGFXMaterialEditor::Generate_AddUVsBoilerplate(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph)
{
	TSoftObjectPtr<UMaterialFunctionInterface> MaterialFunctionPtr(FSoftObjectPath("/Engine/Functions/UserInterface/GetUserInterfaceUV.GetUserInterfaceUV"));
	TObjectPtr<UMaterialFunctionInterface> MaterialFunction = MaterialFunctionPtr.LoadSynchronous();
	const FName CanvasWidthProp(TEXT("CanvasWidth"));
	const float CanvasWidth = OriginalMGFXMaterial->BaseCanvasSize.X;
	const FName CanvasHeightProp(TEXT("CanvasHeight"));
	const float CanvasHeight = OriginalMGFXMaterial->BaseCanvasSize.Y;
	const FLinearColor OutputColor(0.02f, 1.f, 0.7f);


	FVector2D NodePos(GridSize * -64, GridSize * 30);

	// create CanvasWidth and CanvasHeight scalar parameters
	UMaterialExpression* CanvasWidthExp = MNew(UMaterialExpressionScalarParameter, NodePos);
	SET_PROPERTY(CanvasWidthExp, UMaterialExpressionScalarParameter, ParameterName, CanvasWidthProp);
	SET_PROPERTY(CanvasWidthExp, UMaterialExpressionScalarParameter, DefaultValue, CanvasWidth);
	UMaterialExpression* CanvasHeightExp = MNew(UMaterialExpressionScalarParameter, FVector2D(NodePos.X, NodePos.Y + 128));
	SET_PROPERTY(CanvasHeightExp, UMaterialExpressionScalarParameter, ParameterName, CanvasHeightProp);
	SET_PROPERTY(CanvasHeightExp, UMaterialExpressionScalarParameter, DefaultValue, CanvasHeight);

	NodePos.X += GridSize * 20;

	// append width/height
	UMaterialExpression* CanvasAppendExp = MNew(UMaterialExpressionAppendVector, NodePos);
	MConnect(CanvasWidthExp, "", CanvasAppendExp, "A");
	MConnect(CanvasHeightExp, "", CanvasAppendExp, "B");

	// create GetUserInterfaceUVs material function
	UMaterialExpression* InterfaceUVsExp = MNew(UMaterialExpressionMaterialFunctionCall, FVector2D(NodePos.X, NodePos.Y + 256));
	SET_PROPERTY(InterfaceUVsExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, MaterialFunction);

	NodePos.X += GridSize * 20;

	// multiply Normalized UV by canvas size
	UMaterialExpression* MultiplyExp = MNew(UMaterialExpressionMultiply, NodePos);
	MConnect(CanvasAppendExp, "", MultiplyExp, "A");
	MConnect(InterfaceUVsExp, "Normalized UV", MultiplyExp, "B");

	NodePos.X += GridSize * 20;

	// output to canvas UVs reroute
	UMaterialExpression* OutputRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, Reroute_CanvasUVs);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	MConnect(MultiplyExp, "", OutputRerouteExp, "");
}

void FMGFXMaterialEditor::Generate_Shapes(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph)
{
	TSoftObjectPtr<UMaterialFunctionInterface> RotateFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Rotate.MF_MGFX_Rotate"));
	TSoftObjectPtr<UMaterialFunctionInterface> RectFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Rect.MF_MGFX_Shape_Rect"));
	TSoftObjectPtr<UMaterialFunctionInterface> LineFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Line.MF_MGFX_Shape_Line"));
	TObjectPtr<UMaterialFunctionInterface> RotateFunc = RotateFuncPtr.LoadSynchronous();
	TObjectPtr<UMaterialFunctionInterface> RectFunc = RectFuncPtr.LoadSynchronous();
	TObjectPtr<UMaterialFunctionInterface> LineFunc = LineFuncPtr.LoadSynchronous();
	const FLinearColor OutputColor(0.02f, 1.f, 0.7f);

	constexpr float NodePosBaselineLeft = GridSize * -64;
	FVector2D NodePos(NodePosBaselineLeft, GridSize * 80);
	// temporary offset used to simplify next node positioning
	FVector2D NodePosOffset = FVector2D::One();

	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = FindNamedReroute(MaterialGraph, Reroute_CanvasUVs);

	// eventually assign the shapes output for connecting to named reroute
	UMaterialExpression* ShapesOutputExp = nullptr;
	for (int32 Idx = 0; Idx < OriginalMGFXMaterial->ShapeLayers.Num(); ++Idx)
	{
		const FMGFXMaterialShape& Shape = OriginalMGFXMaterial->ShapeLayers[Idx];
		const FString ParamPrefix = Shape.GetName(Idx);
		const FName ParamGroup = FName(FString::Printf(TEXT("[%d] %s"), Idx, *Shape.GetName(Idx)));

		// convert transform to UVs
		UMaterialExpression* ShapeUVsExp = nullptr;
		{
			// TODO: conditionally apply all operations, e.g. only add translate/rotate/scale when desired

			NodePos.X = NodePosBaselineLeft;
			if (Idx > 0)
			{
				NodePos.Y += GridSize * 40;
			}

			// use canvas uvs
			UMaterialExpressionNamedRerouteUsage* CanvasUVsExp = MNew(UMaterialExpressionNamedRerouteUsage, NodePos);
			CanvasUVsExp->Declaration = CanvasUVsDeclaration;

			NodePos.X += GridSize * 15;

			// apply translate
			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionScalarParameter* TranslateXExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
			const FName TranslateXParamName = FName(FString::Printf(TEXT("%s.TranslateX"), *ParamPrefix));
			SET_PROPERTY(TranslateXExp, UMaterialExpressionScalarParameter, ParameterName, TranslateXParamName);
			SET_PROPERTY(TranslateXExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
			SET_PROPERTY_R(TranslateXExp, UMaterialExpressionScalarParameter, SortPriority, 10);
			SET_PROPERTY(TranslateXExp, UMaterialExpressionScalarParameter, DefaultValue, Shape.Transform.Location.X);

			NodePosOffset = FVector2D(0, GridSize * 14);
			UMaterialExpressionScalarParameter* TranslateYExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
			const FName TranslateYParamName = FName(FString::Printf(TEXT("%s.TranslateY"), *ParamPrefix));
			SET_PROPERTY(TranslateYExp, UMaterialExpressionScalarParameter, ParameterName, TranslateYParamName);
			SET_PROPERTY(TranslateYExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
			SET_PROPERTY_R(TranslateYExp, UMaterialExpressionScalarParameter, SortPriority, 11);
			SET_PROPERTY(TranslateYExp, UMaterialExpressionScalarParameter, DefaultValue, Shape.Transform.Location.Y);

			NodePos.X += GridSize * 15;

			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionAppendVector* TranslateAppendExp = MNew(UMaterialExpressionAppendVector, NodePos + NodePosOffset);
			MConnect(TranslateXExp, "", TranslateAppendExp, "A");
			MConnect(TranslateYExp, "", TranslateAppendExp, "B");

			NodePos.X += GridSize * 15;

			// subtract so that coordinate space matches UMG, positive offset means going right or down
			UMaterialExpressionSubtract* TranslateUVsExp = MNew(UMaterialExpressionSubtract, NodePos);
			MConnect(CanvasUVsExp, "", TranslateUVsExp, "A");
			MConnect(TranslateAppendExp, "", TranslateUVsExp, "B");

			NodePos.X += GridSize * 15;


			// apply rotate
			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionScalarParameter* RotationExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
			const FName RotationParamName = FName(FString::Printf(TEXT("%s.Rotation"), *ParamPrefix));
			SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, ParameterName, RotationParamName);
			SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
			SET_PROPERTY_R(RotationExp, UMaterialExpressionScalarParameter, SortPriority, 20);
			SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, DefaultValue, Shape.Transform.Rotation);

			NodePos.X += GridSize * 15;

			// TODO: perf: allow marking rotation as animatable or not and bake in the conversion when not needed
			// conversion from degrees, not baked in to support animation
			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionDivide* RotateConvExp = MNew(UMaterialExpressionDivide, NodePos + NodePosOffset);
			MConnect(RotationExp, "", RotateConvExp, "");
			// negative to match UMG coordinate space, positive rotations are clockwise
			SET_PROPERTY_R(RotateConvExp, UMaterialExpressionDivide, ConstB, -360.f);

			NodePos.X += GridSize * 15;

			UMaterialExpressionMaterialFunctionCall* RotateUVsExp = MNew(UMaterialExpressionMaterialFunctionCall, NodePos);
			SET_PROPERTY(RotateUVsExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, RotateFunc);
			MConnect(TranslateUVsExp, "", RotateUVsExp, "UVs");
			MConnect(RotateConvExp, "", RotateUVsExp, "Rotation");

			NodePos.X += GridSize * 15;


			// apply scale
			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionScalarParameter* ScaleXExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
			const FName ScaleXParamName = FName(FString::Printf(TEXT("%s.ScaleX"), *ParamPrefix));
			SET_PROPERTY(ScaleXExp, UMaterialExpressionScalarParameter, ParameterName, ScaleXParamName);
			SET_PROPERTY(ScaleXExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
			SET_PROPERTY_R(ScaleXExp, UMaterialExpressionScalarParameter, SortPriority, 30);
			SET_PROPERTY(ScaleXExp, UMaterialExpressionScalarParameter, DefaultValue, Shape.Transform.Scale.X);

			NodePosOffset = FVector2D(0, GridSize * 14);
			UMaterialExpressionScalarParameter* ScaleYExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
			const FName ScaleYParamName = FName(FString::Printf(TEXT("%s.ScaleY"), *ParamPrefix));
			SET_PROPERTY(ScaleYExp, UMaterialExpressionScalarParameter, ParameterName, ScaleYParamName);
			SET_PROPERTY(ScaleYExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
			SET_PROPERTY_R(ScaleYExp, UMaterialExpressionScalarParameter, SortPriority, 31);
			SET_PROPERTY(ScaleYExp, UMaterialExpressionScalarParameter, DefaultValue, Shape.Transform.Scale.Y);

			NodePos.X += GridSize * 15;

			NodePosOffset = FVector2D(0, GridSize * 8);
			UMaterialExpressionAppendVector* ScaleAppendExp = MNew(UMaterialExpressionAppendVector, NodePos + NodePosOffset);
			MConnect(ScaleXExp, "", ScaleAppendExp, "A");
			MConnect(ScaleYExp, "", ScaleAppendExp, "B");

			NodePos.X += GridSize * 15;

			UMaterialExpressionDivide* ScaleUVsExp = MNew(UMaterialExpressionDivide, NodePos);
			MConnect(RotateUVsExp, "", ScaleUVsExp, "A");
			MConnect(ScaleAppendExp, "", ScaleUVsExp, "B");

			NodePos.X += GridSize * 15;

			ShapeUVsExp = ScaleUVsExp;
		}


		// create shape
		NodePosOffset = FVector2D(0, GridSize * 8);
		// currently not animatable
		UMaterialExpressionConstant2Vector* ShapeSizeExp = MNew(UMaterialExpressionConstant2Vector, NodePos + NodePosOffset);
		SET_PROPERTY(ShapeSizeExp, UMaterialExpressionConstant2Vector, R, Shape.Size.X);
		SET_PROPERTY(ShapeSizeExp, UMaterialExpressionConstant2Vector, G, Shape.Size.Y);

		NodePos.X += GridSize * 15;

		UMaterialExpressionMaterialFunctionCall* ShapeExp = MNew(UMaterialExpressionMaterialFunctionCall, NodePos);
		// TODO: dynamically select shape func
		UMaterialFunctionInterface* ShapeFunc = RectFunc;
		SET_PROPERTY(ShapeExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, ShapeFunc);
		if (ShapeUVsExp)
		{
			MConnect(ShapeUVsExp, "", ShapeExp, "UVs");
		}

		// TODO: dynamically connect shape inputs which will vary per shape
		MConnect(ShapeSizeExp, "", ShapeExp, "Size");

		NodePos.X += GridSize * 15;


		// combine with previous layer
		if (ShapesOutputExp)
		{
			UMaterialExpressionAdd* ShapeLayerMergeExp = MNew(UMaterialExpressionAdd, NodePos);
			MConnect(ShapesOutputExp, "", ShapeLayerMergeExp, "A");
			MConnect(ShapeExp, "", ShapeLayerMergeExp, "B");

			ShapesOutputExp = ShapeLayerMergeExp;
		}
		else
		{
			ShapesOutputExp = ShapeExp;
		}
	}

	NodePos.X += GridSize * 15;

	// create shapes output reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, Reroute_ShapesOutput);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	MConnect(ShapesOutputExp, "", OutputRerouteExp, "");
}

UMaterialExpressionNamedRerouteDeclaration* FMGFXMaterialEditor::FindNamedReroute(UMaterialGraph* MaterialGraph, FName Name) const
{
	for (UEdGraphNode* Node : MaterialGraph->Nodes)
	{
		if (const UMaterialGraphNode* MaterialNode = Cast<UMaterialGraphNode>(Node))
		{
			UMaterialExpressionNamedRerouteDeclaration* Declaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(MaterialNode->MaterialExpression);
			if (Declaration && Declaration->Name.IsEqual(Name))
			{
				return Declaration;
			}
		}
	}
	return nullptr;
}


#undef LOCTEXT_NAMESPACE

#undef SET_PROPERTY
#undef MNew
#undef MConnect
