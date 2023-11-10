// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "IMaterialEditor.h"
#include "MaterialEditingLibrary.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditorCommands.h"
#include "MaterialGraph/MaterialGraph.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "UObject/PropertyAccessUtil.h"

#define SET_EDITOR_PROPERTY(Object, Class, PropName, Value) \
	{ \
		const FProperty* Prop = PropertyAccessUtil::FindPropertyByName( \
			GET_MEMBER_NAME_CHECKED(Class, PropName), Class::StaticClass()); \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, &Value, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"

constexpr auto ReadOnlyFlags = PropertyAccessUtil::EditorReadOnlyFlags;
constexpr auto NotifyMode = EPropertyAccessChangeNotifyMode::Default;

const FName FMGFXMaterialEditor::DetailsTabId(TEXT("MGFXMaterialEditorDetailsTab"));
const FName FMGFXMaterialEditor::CanvasTabId(TEXT("MGFXMaterialEditorCanvasTab"));

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

	{
		FVector2D Position = FVector2D(-256, 0);

		// connect emissive to multiply
		UMaterialExpression* Multiply = MaterialEditor->CreateNewMaterialExpression(
			UMaterialExpressionMultiply::StaticClass(), Position, false, false);
		Position -= FVector2D(320, 0);
		UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, FString(), MP_EmissiveColor);

		// create a constant color
		UMaterialExpression* ConstantColor = MaterialEditor->CreateNewMaterialExpression(
			UMaterialExpressionConstant3Vector::StaticClass(), Position, false, false);
		Position -= FVector2D(320, 0);
		// set random color
		const FLinearColor RandomColor = FLinearColor::MakeRandomColor();
		const FProperty* ConstantProperty = PropertyAccessUtil::FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(UMaterialExpressionConstant3Vector, Constant),
			UMaterialExpressionConstant3Vector::StaticClass());
		PropertyAccessUtil::SetPropertyValue_Object(ConstantProperty, ConstantColor, ConstantProperty, &RandomColor, INDEX_NONE, ReadOnlyFlags, NotifyMode);

		// connect to multiply
		UMaterialEditingLibrary::ConnectMaterialExpressions(ConstantColor, FString(), Multiply, FString());
	}

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

	UMaterialExpressionComment* CommentExp = MaterialEditor->CreateNewMaterialExpressionComment(FVector2D(-512, -256));

	const FProperty* ColorProp = PropertyAccessUtil::FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMaterialExpressionComment, CommentColor), UMaterialExpressionComment::StaticClass());
	PropertyAccessUtil::SetPropertyValue_Object(ColorProp, CommentExp, ColorProp, &CommentColor, INDEX_NONE, ReadOnlyFlags, NotifyMode);

	const FProperty* TextProp = PropertyAccessUtil::FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMaterialExpressionComment, Text), UMaterialExpressionComment::StaticClass());
	PropertyAccessUtil::SetPropertyValue_Object(TextProp, CommentExp, TextProp, &Text, INDEX_NONE, ReadOnlyFlags, NotifyMode);
}

void FMGFXMaterialEditor::Generate_AddUVsBoilerplate(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph)
{
	TSoftObjectPtr<UMaterialFunctionInterface> MaterialFunctionPtr(FSoftObjectPath("/Engine/Functions/UserInterface/GetUserInterfaceUV.GetUserInterfaceUV"));
	TObjectPtr<UMaterialFunctionInterface> MaterialFunction = MaterialFunctionPtr.LoadSynchronous();
	const FName OutputRerouteName(TEXT("CanvasUVs"));
	const FLinearColor OutputColor(0.02f, 1.f, 0.7f);

	FVector2D NodePos = FVector2D(-1024, 512);


	// create CanvasWidth and CanvasHeight scalar parameters
	UMaterialExpression* CanvasWidthExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionScalarParameter::StaticClass(),
		NodePos, false, false);
	UMaterialExpression* CanvasHeightExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionScalarParameter::StaticClass(),
		FVector2D(NodePos.X, NodePos.Y + 128), false, false);

	NodePos.X += 320;

	// append width/height
	auto* CanvasAppendExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionAppendVector::StaticClass(),
		NodePos, false, false);
	UMaterialEditingLibrary::ConnectMaterialExpressions(CanvasWidthExp, FString(), CanvasAppendExp, FString("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(CanvasHeightExp, FString(), CanvasAppendExp, FString("B"));

	// create GetUserInterfaceUVs material function
	UMaterialExpression* InterfaceUVsExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionMaterialFunctionCall::StaticClass(),
		FVector2D(NodePos.X, NodePos.Y + 256), false, false);
	const FProperty* FunctionProp = PropertyAccessUtil::FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMaterialExpressionMaterialFunctionCall, MaterialFunction), UMaterialExpressionMaterialFunctionCall::StaticClass());
	PropertyAccessUtil::SetPropertyValue_Object(FunctionProp, InterfaceUVsExp, FunctionProp, &MaterialFunction, INDEX_NONE, ReadOnlyFlags, NotifyMode);

	NodePos.X += 320;

	// multiply Normalized UV by canvas size
	UMaterialExpression* MultiplyExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionMultiply::StaticClass(),
		NodePos, false, false);
	UMaterialEditingLibrary::ConnectMaterialExpressions(CanvasAppendExp, FString(), MultiplyExp, FString("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(InterfaceUVsExp, FString("Normalized UV"), MultiplyExp, FString("B"));

	NodePos.X += 320;

	// output to named reroute
	UMaterialExpression* OutputExp = MaterialEditor->CreateNewMaterialExpression(
		UMaterialExpressionNamedRerouteDeclaration::StaticClass(),
		NodePos, false, false);
	SET_EDITOR_PROPERTY(OutputExp, UMaterialExpressionNamedRerouteDeclaration, Name, OutputRerouteName);
	SET_EDITOR_PROPERTY(OutputExp, UMaterialExpressionNamedRerouteDeclaration, NodeColor, OutputColor);
	PropertyAccessUtil::SetPropertyValue_Object(FunctionProp, InterfaceUVsExp, FunctionProp, &MaterialFunction, INDEX_NONE, ReadOnlyFlags, NotifyMode);
	UMaterialEditingLibrary::ConnectMaterialExpressions(MultiplyExp, FString(), OutputExp, FString());
}


#undef LOCTEXT_NAMESPACE
