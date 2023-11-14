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
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Shapes/MGFXMaterialShape.h"
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
	Cast<Class>(Builder.MaterialEditor->CreateNewMaterialExpression( \
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
const FName FMGFXMaterialEditor::Reroute_FilterWidth(TEXT("FilterWidth"));
const FName FMGFXMaterialEditor::Reroute_ShapesOutput(TEXT("ShapesOutput"));
const int32 FMGFXMaterialEditor::GridSize(16);


// FMGFXMaterialBuilder
// --------------------

FMGFXMaterialBuilder::FMGFXMaterialBuilder(IMaterialEditor* InMaterialEditor, UMaterialGraph* InMaterialGraph)
	: MaterialEditor(InMaterialEditor),
	  MaterialGraph(InMaterialGraph)
{
}


// FMGFXMaterialEditor
// -------------------

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

	FMGFXMaterialBuilder Builder(MaterialEditor, MaterialGraph);

	Generate_DeleteAllNodes(Builder);
	Generate_AddWarningComment(Builder);
	Generate_AddUVsBoilerplate(Builder);
	Generate_Shapes(Builder);

	// connect shapes output to opacity
	UMaterialExpressionNamedRerouteDeclaration* ShapesOutputRerouteExp = FindNamedReroute(MaterialGraph, Reroute_ShapesOutput);
	check(ShapesOutputRerouteExp);

	UMaterialExpressionNamedRerouteUsage* OutputUsageExp = MNew(UMaterialExpressionNamedRerouteUsage, FVector2D(GridSize * -16, 0));
	OutputUsageExp->Declaration = ShapesOutputRerouteExp;
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

void FMGFXMaterialEditor::Generate_DeleteAllNodes(const FMGFXMaterialBuilder& Builder)
{
	auto NodesToDelete = Builder.MaterialGraph->Nodes.FilterByPredicate([](const UEdGraphNode* Node)
	{
		return Node->CanUserDeleteNode();
	});
	Builder.MaterialEditor->DeleteNodes(NodesToDelete);
}

void FMGFXMaterialEditor::Generate_AddWarningComment(const FMGFXMaterialBuilder& Builder)
{
	const FLinearColor CommentColor = FLinearColor(0.06f, 0.02f, 0.02f);
	const FString Text = FString::Printf(TEXT("Generated by %s\nDo not edit manually"), *GetNameSafe(OriginalMGFXMaterial));

	FVector2D NodePos(GridSize * -20, GridSize * -10);
	UMaterialExpressionComment* CommentExp = Builder.MaterialEditor->CreateNewMaterialExpressionComment(NodePos);
	SET_PROPERTY(CommentExp, UMaterialExpressionComment, CommentColor, CommentColor);
	SET_PROPERTY(CommentExp, UMaterialExpressionComment, Text, Text);
}

void FMGFXMaterialEditor::Generate_AddUVsBoilerplate(const FMGFXMaterialBuilder& Builder)
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

	NodePos.X += GridSize * 15;

	// append width/height
	UMaterialExpression* CanvasAppendExp = MNew(UMaterialExpressionAppendVector, NodePos);
	MConnect(CanvasWidthExp, "", CanvasAppendExp, "A");
	MConnect(CanvasHeightExp, "", CanvasAppendExp, "B");

	NodePos.X += GridSize * 15;

	// create GetUserInterfaceUVs material function
	UMaterialExpression* InterfaceUVsExp = MNew(UMaterialExpressionMaterialFunctionCall, FVector2D(NodePos.X, NodePos.Y + GridSize * 8));
	SET_PROPERTY(InterfaceUVsExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, MaterialFunction);

	NodePos.X += GridSize * 15;

	// multiply Normalized UV by canvas size
	UMaterialExpression* MultiplyExp = MNew(UMaterialExpressionMultiply, NodePos);
	MConnect(CanvasAppendExp, "", MultiplyExp, "A");
	MConnect(InterfaceUVsExp, "Normalized UV", MultiplyExp, "B");

	NodePos.X += GridSize * 15;

	// output to canvas UVs reroute
	UMaterialExpression* OutputRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, Reroute_CanvasUVs);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	MConnect(MultiplyExp, "", OutputRerouteExp, "");

	NodePos.X += GridSize * 15;

	// compute common filter width to use for all shapes based on canvas resolution
	UMaterialExpression* FilterWidthExp = nullptr;

	if (OriginalMGFXMaterial->bComputeFilterWidth)
	{
		TSoftObjectPtr<UMaterialFunctionInterface> FilterWidthFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_FilterWidth.MF_MGFX_FilterWidth"));
		TObjectPtr<UMaterialFunctionInterface> FilterWidthFunc = FilterWidthFuncPtr.LoadSynchronous();

		FilterWidthExp = MNew(UMaterialExpressionMaterialFunctionCall, NodePos);
		SET_PROPERTY(FilterWidthExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, FilterWidthFunc);
		MConnect(OutputRerouteExp, "", FilterWidthExp, "");

		NodePos.X += GridSize * 15;

		// apply filter width scale if not 1x
		if (!FMath::IsNearlyEqual(OriginalMGFXMaterial->FilterWidthScale, 1.f))
		{
			UMaterialExpressionMultiply* FilterWidthScaleExp = MNew(UMaterialExpressionMultiply, NodePos);
			MConnect(FilterWidthExp, "", FilterWidthScaleExp, "");
			SET_PROPERTY(FilterWidthScaleExp, UMaterialExpressionMultiply, ConstB, OriginalMGFXMaterial->FilterWidthScale);

			NodePos.X += GridSize * 15;

			FilterWidthExp = FilterWidthScaleExp;
		}
	}
	else
	{
		FilterWidthExp = MNew(UMaterialExpressionScalarParameter, NodePos);
		SET_PROPERTY(FilterWidthExp, UMaterialExpressionScalarParameter, DefaultValue, OriginalMGFXMaterial->FilterWidthScale);

		NodePos.X += GridSize * 15;
	}

	// add filter width reroute
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY(FilterWidthRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, Reroute_FilterWidth);
	SET_PROPERTY(FilterWidthRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	MConnect(FilterWidthExp, "", FilterWidthRerouteExp, "");
}

void FMGFXMaterialEditor::Generate_Shapes(const FMGFXMaterialBuilder& Builder)
{
	const FLinearColor OutputColor(0.22f, .09f, 0.55f);

	constexpr float NodePosBaselineLeft = GridSize * -64;
	FVector2D NodePos(NodePosBaselineLeft, GridSize * 80);

	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = FindNamedReroute(Builder.MaterialGraph, Reroute_CanvasUVs);

	// eventually assign the shapes output for connecting to named reroute
	UMaterialExpression* ShapesOutputExp = nullptr;
	for (int32 Idx = 0; Idx < OriginalMGFXMaterial->Layers.Num(); ++Idx)
	{
		const FMGFXMaterialLayer& Layer = OriginalMGFXMaterial->Layers[Idx];

		const FString ParamPrefix = Layer.GetName(Idx);
		const FName ParamGroup = FName(FString::Printf(TEXT("[%d] %s"), Idx, *Layer.GetName(Idx)));

		NodePos.X = NodePosBaselineLeft;
		if (Idx > 0)
		{
			NodePos.Y += GridSize * 40;
		}

		// convert transform to UVs
		UMaterialExpressionNamedRerouteUsage* CanvasUVsExp = MNew(UMaterialExpressionNamedRerouteUsage, NodePos);
		CanvasUVsExp->Declaration = CanvasUVsDeclaration;

		NodePos.X += GridSize * 15;

		UMaterialExpression* LayerUVsExp = Generate_TransformUVs(Builder, NodePos, Layer.Transform, CanvasUVsExp, ParamPrefix, ParamGroup);

		// create shape
		if (Layer.Shape)
		{
			UMaterialExpression* NewShapeExp = Generate_Shape(Builder, NodePos, Layer.Shape, LayerUVsExp, ParamPrefix, ParamGroup);

			// merge with previous shape layer
			if (ShapesOutputExp)
			{
				ShapesOutputExp = Generate_MergeShapes(Builder, NodePos, ShapesOutputExp, NewShapeExp, ParamPrefix, ParamGroup);
			}
			else
			{
				ShapesOutputExp = NewShapeExp;
			}
		}
	}

	NodePos.X += GridSize * 15;

	// create shapes output reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, Reroute_ShapesOutput);
	SET_PROPERTY(OutputRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	MConnect(ShapesOutputExp, "", OutputRerouteExp, "");
}

UMaterialExpression* FMGFXMaterialEditor::Generate_TransformUVs(const FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
                                                                const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
                                                                const FString& ParamPrefix, const FName& ParamGroup, bool bCreateReroute)
{
	TSoftObjectPtr<UMaterialFunctionInterface> RotateFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Rotate.MF_MGFX_Rotate"));
	TObjectPtr<UMaterialFunctionInterface> RotateFunc = RotateFuncPtr.LoadSynchronous();

	const FLinearColor UVsOutputColor(0.02f, 1.f, 0.7f);

	// temporary offset used to simplify next node positioning
	FVector2D NodePosOffset = FVector2D::Zero();

	// TODO: conditionally apply all operations, e.g. only add translate/rotate/scale when desired

	// apply translate
	NodePos.Y += GridSize * 8;
	UMaterialExpression* TranslateValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Location, ParamPrefix, ParamGroup,
	                                                                   10, "TranslateX", "TranslateY");
	NodePos.Y -= GridSize * 8;

	// subtract so that coordinate space matches UMG, positive offset means going right or down
	UMaterialExpressionSubtract* TranslateUVsExp = MNew(UMaterialExpressionSubtract, NodePos);
	MConnect(InUVsExp, "", TranslateUVsExp, "A");
	MConnect(TranslateValueExp, "", TranslateUVsExp, "B");

	NodePos.X += GridSize * 15;


	// apply rotate
	NodePosOffset = FVector2D(0, GridSize * 8);
	UMaterialExpressionScalarParameter* RotationExp = MNew(UMaterialExpressionScalarParameter, NodePos + NodePosOffset);
	const FName RotationParamName = FName(FString::Printf(TEXT("%s.Rotation"), *ParamPrefix));
	SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, ParameterName, RotationParamName);
	SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
	SET_PROPERTY_R(RotationExp, UMaterialExpressionScalarParameter, SortPriority, 20);
	SET_PROPERTY(RotationExp, UMaterialExpressionScalarParameter, DefaultValue, Transform.Rotation);

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
	NodePos.Y += GridSize * 8;
	UMaterialExpression* ScaleValueExp = Generate_Vector2Parameter(Builder, NodePos, Transform.Scale, ParamPrefix, ParamGroup,
	                                                               30, "ScaleX", "ScaleY");
	NodePos.Y -= GridSize * 8;

	UMaterialExpressionDivide* ScaleUVsExp = MNew(UMaterialExpressionDivide, NodePos);
	MConnect(RotateUVsExp, "", ScaleUVsExp, "A");
	MConnect(ScaleValueExp, "", ScaleUVsExp, "B");

	NodePos.X += GridSize * 15;

	if (!bCreateReroute)
	{
		// early exit, don't create reroute
		return ScaleUVsExp;
	}

	// output to named reroute
	UMaterialExpressionNamedRerouteDeclaration* UVsRerouteExp = MNew(UMaterialExpressionNamedRerouteDeclaration, NodePos);
	SET_PROPERTY_R(UVsRerouteExp, UMaterialExpressionNamedRerouteDeclaration, Name, FName(FString::Printf(TEXT("%sUVs"), *ParamPrefix)));
	SET_PROPERTY(UVsRerouteExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, UVsOutputColor);
	MConnect(ScaleUVsExp, "", UVsRerouteExp, "");

	NodePos.X += GridSize * 15;

	return UVsRerouteExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_Shape(const FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
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

		switch (Input.Type)
		{
		case EMGFXMaterialShapeInputType::Float:
			SET_PROPERTY(InputExp, UMaterialExpressionConstant, R, Value.R);
			break;
		case EMGFXMaterialShapeInputType::Vector2:
			SET_PROPERTY(InputExp, UMaterialExpressionConstant2Vector, R, Value.R);
			SET_PROPERTY(InputExp, UMaterialExpressionConstant2Vector, G, Value.G);
			break;
		case EMGFXMaterialShapeInputType::Vector3:
			SET_PROPERTY(InputExp, UMaterialExpressionConstant3Vector, Constant, Value);
			break;
		case EMGFXMaterialShapeInputType::Vector4:
			SET_PROPERTY(InputExp, UMaterialExpressionConstant4Vector, Constant, Value);
			break;
		}

		NodePosOffset.Y += GridSize * 8;
	}
	check(Inputs.Num() == InputExps.Num());

	// include filter width input
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthReroute = FindNamedReroute(Builder.MaterialGraph, Reroute_FilterWidth);
	UMaterialExpressionNamedRerouteUsage* FilterWidthExp = MNew(UMaterialExpressionNamedRerouteUsage, NodePos + NodePosOffset);
	FilterWidthExp->Declaration = FilterWidthReroute;

	NodePos.X += GridSize * 20;

	// create shape function
	UMaterialExpressionMaterialFunctionCall* ShapeFuncExp = MNew(UMaterialExpressionMaterialFunctionCall, NodePos);
	UMaterialFunctionInterface* ShapeFunc = Shape->GetMaterialFunction();
	check(ShapeFunc);
	SET_PROPERTY(ShapeFuncExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, ShapeFunc);
	// connect uvs
	if (InUVsExp)
	{
		MConnect(InUVsExp, "", ShapeFuncExp, "UVs");
	}

	// connect inputs
	for (int32 Idx = 0; Idx < Inputs.Num(); ++Idx)
	{
		const FMGFXMaterialShapeInput& Input = Inputs[Idx];
		UMaterialExpression* InputExp = InputExps[Idx];

		MConnect(InputExp, "", ShapeFuncExp, Input.Name);
	}

	// connect filter width
	MConnect(FilterWidthExp, "", ShapeFuncExp, "FilterWidth");

	NodePos.X += GridSize * 15;

	return ShapeFuncExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_MergeShapes(const FMGFXMaterialBuilder& Builder, FVector2D& NodePos, UMaterialExpression* ShapeAExp,
                                                               UMaterialExpression* ShapeBExp, const FString& ParamPrefix, const FName& ParamGroup)
{
	// TODO: more merge operations
	UMaterialExpressionAdd* ShapeMergeExp = MNew(UMaterialExpressionAdd, NodePos);
	MConnect(ShapeAExp, "", ShapeMergeExp, "A");
	MConnect(ShapeBExp, "", ShapeMergeExp, "B");

	return ShapeMergeExp;
}

UMaterialExpression* FMGFXMaterialEditor::Generate_Vector2Parameter(const FMGFXMaterialBuilder& Builder, FVector2D& NodePos, FVector2f DefaultValue,
                                                                    const FString& ParamPrefix, const FName& ParamGroup, int32 BaseSortPriority,
                                                                    const FString& ParamNameX, const FString& ParamNameY)
{
	UMaterialExpressionScalarParameter* XExp = MNew(UMaterialExpressionScalarParameter, NodePos);
	const FName ParamNameXFull = FName(FString::Printf(TEXT("%s.%s"), *ParamPrefix, *ParamNameX));
	SET_PROPERTY(XExp, UMaterialExpressionScalarParameter, ParameterName, ParamNameXFull);
	SET_PROPERTY(XExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
	SET_PROPERTY_R(XExp, UMaterialExpressionScalarParameter, SortPriority, BaseSortPriority);
	SET_PROPERTY(XExp, UMaterialExpressionScalarParameter, DefaultValue, DefaultValue.X);

	UMaterialExpressionScalarParameter* YExp = MNew(UMaterialExpressionScalarParameter, NodePos + FVector2D(0, GridSize * 6));
	const FName ParamNameYFull = FName(FString::Printf(TEXT("%s.%s"), *ParamPrefix, *ParamNameY));
	SET_PROPERTY(YExp, UMaterialExpressionScalarParameter, ParameterName, ParamNameYFull);
	SET_PROPERTY(YExp, UMaterialExpressionScalarParameter, Group, ParamGroup);
	SET_PROPERTY_R(YExp, UMaterialExpressionScalarParameter, SortPriority, BaseSortPriority + 1);
	SET_PROPERTY(YExp, UMaterialExpressionScalarParameter, DefaultValue, DefaultValue.Y);

	NodePos.X += GridSize * 15;

	// append
	UMaterialExpressionAppendVector* AppendExp = MNew(UMaterialExpressionAppendVector, NodePos);
	MConnect(XExp, "", AppendExp, "A");
	MConnect(YExp, "", AppendExp, "B");

	NodePos.X += GridSize * 15;

	return AppendExp;
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
