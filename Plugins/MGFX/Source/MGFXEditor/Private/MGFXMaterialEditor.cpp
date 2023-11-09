// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditor.h"

#include "MaterialEditorUtilities.h"
#include "MGFXEditorModule.h"
#include "MGFXMaterial.h"
#include "MaterialGraph/MaterialGraph.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"


void FMGFXMaterialEditor::InitMGFXMaterialEditor(const EToolkitMode::Type Mode,
                                                 const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                 UMGFXMaterial* InMGFXMaterial)
{
	check(InMGFXMaterial);

	OriginalMGFXMaterial = InMGFXMaterial;

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
					->AddTab("MGFXMaterialEditorCanvasTab", ETabState::OpenedTab)
				)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->AddTab("MGFMaterialEditorDetailsTab", ETabState::OpenedTab)
				)
			)
		);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FMGFXEditorModule::MGFXMaterialEditorAppIdentifier, StandaloneDefaultLayout,
	                bCreateDefaultStandaloneMenu, bCreateDefaultToolbar,
	                OriginalMGFXMaterial);
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

	InTabManager->RegisterTabSpawner("MGFXMaterialEditorCanvasTab", FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Canvas))
	          .SetDisplayName(LOCTEXT("CanvasTabTitle", "Canvas"));


	InTabManager->RegisterTabSpawner("MGFXMaterialEditorDetailsTab", FOnSpawnTab::CreateSP(this, &FMGFXMaterialEditor::SpawnTab_Details))
	          .SetDisplayName(LOCTEXT("DetailsTabTitle", "Details"));
}

void FMGFXMaterialEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	IMGFXMaterialEditor::UnregisterTabSpawners(InTabManager);
}

UMaterial* FMGFXMaterialEditor::GetGeneratedMaterial() const
{
	return OriginalMGFXMaterial ? OriginalMGFXMaterial->GeneratedMaterial : nullptr;
}

void FMGFXMaterialEditor::TestGenerate()
{
	const UMaterial* Material = GetGeneratedMaterial();
	if (!Material)
	{
		return;
	}
	const UMaterialGraph* MaterialGraph = Material->MaterialGraph;

	FMaterialEditorUtilities::CreateNewMaterialExpressionComment(MaterialGraph, FVector2D::ZeroVector);
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Canvas(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SOverlay)
	];
}

TSharedRef<SDockTab> FMGFXMaterialEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(OriginalMGFXMaterial);

	return SNew(SDockTab)
	[
		DetailsView
	];
}

#undef LOCTEXT_NAMESPACE
