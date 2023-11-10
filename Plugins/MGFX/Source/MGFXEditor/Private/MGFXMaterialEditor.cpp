// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "IMaterialEditor.h"
#include "MaterialEditingLibrary.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
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
	TSoftObjectPtr<UMaterialFunctionInterface> RectFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Rect.MF_MGFX_Shape_Rect"));
	TSoftObjectPtr<UMaterialFunctionInterface> LineFuncPtr(FSoftObjectPath("/MGFX/MaterialFunctions/MF_MGFX_Shape_Line.MF_MGFX_Shape_Line"));
	TObjectPtr<UMaterialFunctionInterface> RectFunc = RectFuncPtr.LoadSynchronous();
	TObjectPtr<UMaterialFunctionInterface> LineFunc = LineFuncPtr.LoadSynchronous();
	const FLinearColor OutputColor(0.02f, 1.f, 0.7f);

	FVector2D NodePos(GridSize * -64, GridSize * 80);

	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = FindNamedReroute(MaterialGraph, Reroute_CanvasUVs);

	// eventually assign the shapes output for connecting to named reroute
	UMaterialExpression* ShapesOutputExp = nullptr;
	{
		// TODO: convert a 2d shape transform into a node graph
		// use canvas uvs
		UMaterialExpressionNamedRerouteUsage* CanvasUVsExp = MNew(UMaterialExpressionNamedRerouteUsage, NodePos);
		CanvasUVsExp->Declaration = CanvasUVsDeclaration;

		float RectSizeX = 50.f;
		float RectSizeY = 50.f;
		UMaterialExpressionConstant2Vector* RectSizeExp = MNew(UMaterialExpressionConstant2Vector, FVector2D(NodePos.X, NodePos.Y + GridSize * 8));
		SET_PROPERTY(RectSizeExp, UMaterialExpressionConstant2Vector, R, RectSizeX);
		SET_PROPERTY(RectSizeExp, UMaterialExpressionConstant2Vector, G, RectSizeY);

		NodePos.X += GridSize * 20;

		// create some test shapes
		UMaterialExpressionMaterialFunctionCall* TestRectExp = MNew(UMaterialExpressionMaterialFunctionCall, NodePos);
		SET_PROPERTY(TestRectExp, UMaterialExpressionMaterialFunctionCall, MaterialFunction, RectFunc);
		MConnect(CanvasUVsExp, "", TestRectExp, "UVs");
		MConnect(RectSizeExp, "", TestRectExp, "Size");

		NodePos.X += GridSize * 20;

		ShapesOutputExp = TestRectExp;
	}

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
