// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMGFXMaterialEditor.h"

class UMaterialExpressionNamedRerouteDeclaration;
class IMaterialEditor;
class UMGFXMaterial;


/**
 * Motion graphics editor for MGFX materials.
 */
class MGFXEDITOR_API FMGFXMaterialEditor : public IMGFXMaterialEditor
{
public:
	/** Initialize the editor for a material. */
	void InitMGFXMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMGFXMaterial* InMGFXMaterial);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/** Return the generated material asset being edited. */
	UMaterial* GetGeneratedMaterial() const;

	/** Fully regenerate the target material. */
	void RegenerateMaterial();

private:
	/** The original MGFX material asset being edited. */
	TObjectPtr<UMGFXMaterial> OriginalMGFXMaterial;

	void BindCommands();
	void ExtendToolbar();

	TSharedRef<SDockTab> SpawnTab_Canvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);

	/** Delete all non-root nodes from a material graph. */
	void Generate_DeleteAllNodes(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph);

	/** Add a generated warning comment to prevent user modification. */
	void Generate_AddWarningComment(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph);

	/** Create boilerplate UVs based on desired canvas size. */
	void Generate_AddUVsBoilerplate(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph);

	/** Generate all shape layers and combine them. */
	void Generate_Shapes(IMaterialEditor* MaterialEditor, UMaterialGraph* MaterialGraph);

	/** Find and return a named reroute declaration by name. */
	UMaterialExpressionNamedRerouteDeclaration* FindNamedReroute(UMaterialGraph* MaterialGraph, FName Name) const;

public:
	static const FName DetailsTabId;
	static const FName CanvasTabId;
	static const int32 GridSize;
};
