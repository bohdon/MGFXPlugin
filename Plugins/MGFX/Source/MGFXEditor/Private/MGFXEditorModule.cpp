#include "MGFXEditorModule.h"

#include "MGFXEditorStyle.h"
#include "MGFXMaterialEditor.h"


#define LOCTEXT_NAMESPACE "FMGFXEditorModule"


const FName FMGFXEditorModule::MGFXMaterialEditorAppIdentifier(TEXT("MGFXMaterialEditorApp"));

void FMGFXEditorModule::StartupModule()
{
	FMGFXEditorStyle::Get();
}

void FMGFXEditorModule::ShutdownModule()
{
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
