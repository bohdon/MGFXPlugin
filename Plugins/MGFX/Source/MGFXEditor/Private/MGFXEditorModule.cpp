#include "MGFXEditorModule.h"

#include "MGFXEditorStyle.h"
#include "MGFXMaterialEditor.h"
#include "ThumbnailRendering/MGFXMaterialThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"


DEFINE_LOG_CATEGORY(LogMGFXEditor);


#define LOCTEXT_NAMESPACE "FMGFXEditorModule"


const FName FMGFXEditorModule::MGFXMaterialEditorAppIdentifier(TEXT("MGFXMaterialEditorApp"));

void FMGFXEditorModule::StartupModule()
{
	FMGFXEditorStyle::Get();

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMGFXEditorModule::OnPostEngineInit);
}

void FMGFXEditorModule::ShutdownModule()
{
}

void FMGFXEditorModule::OnPostEngineInit()
{
	if (GIsEditor)
	{
		UThumbnailManager::Get().RegisterCustomRenderer(
			UMGFXMaterial::StaticClass(),
			UMGFXMaterialThumbnailRenderer::StaticClass());
	}
}

TSharedRef<IMGFXMaterialEditor> FMGFXEditorModule::CreateMGFXMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                                            UMGFXMaterial* MGFXMaterial)
{
	TSharedRef<FMGFXMaterialEditor> NewEditor(new FMGFXMaterialEditor());
	NewEditor->InitMGFXMaterialEditor(Mode, InitToolkitHost, MGFXMaterial);
	return NewEditor;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMGFXEditorModule, MGFXEditor)
