#pragma once

#include "CoreMinimal.h"
#include "IMGFXMaterialEditor.h"
#include "MGFXMaterial.h"
#include "Modules/ModuleManager.h"


DECLARE_LOG_CATEGORY_EXTERN(LogMGFXEditor, Log, All);


class FMGFXEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Create a new MGFX material editor. */
	TSharedRef<IMGFXMaterialEditor> CreateMGFXMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
	                                                         UMGFXMaterial* MGFXMaterial);

	static const FName MGFXMaterialEditorAppIdentifier;
};
