// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "IMGFXMaterialEditor.h"
#include "MGFXMaterialTypes.h"
#include "MaterialEditor/PreviewMaterial.h"
#include "Misc/NotifyHook.h"

class FMGFXMaterialGenerator;
class IMaterialEditor;
class SMGFXMaterialEditorCanvas;
class SMGFXMaterialEditorLayers;
class UMGFXMaterial;
class UMGFXMaterialLayer;
class UPreviewMaterial;

#define GENERATOR_REFACTOR 1


/**
 * Motion graphics editor for MGFX materials.
 */
class MGFXEDITOR_API FMGFXMaterialEditor : public IMGFXMaterialEditor,
                                           public FGCObject,
                                           public FNotifyHook,
                                           public FEditorUndoClient
{
public:
	FMGFXMaterialEditor();

	virtual ~FMGFXMaterialEditor() override;


	/** Initialize the editor for a material. */
	void InitMGFXMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMGFXMaterial* InMGFXMaterial);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/** Return the MGFX Material asset being edited. */
	UMGFXMaterial* GetMGFXMaterial() const { return MGFXMaterial; }

	/** Return the material asset being generated. */
	UMaterial* GetGeneratedMaterial() const;

	/** Return the dynamic instance of the material asset being generated for previewing changes. */
	UMaterialInstanceDynamic* GetPreviewMaterial() const { return PreviewMaterial; }

	/** Fully regenerate the target material. */
	void RegenerateMaterial();

	void ToggleAutoRegenerate();

	bool IsAutoRegenerateEnabled() const { return bAutoRegenerate; }

	/** Create a new material asset for the MGFX material. */
	UMaterial* CreateMaterialAsset();

	FVector2D GetCanvasSize() const;

	TArray<TObjectPtr<UMGFXMaterialLayer>> GetSelectedLayers() const;

	void SetSelectedLayers(const TArray<UMGFXMaterialLayer*>& Layers);

	void ClearSelectedLayers();

	void OnLayerSelectionChanged(TArray<TObjectPtr<UMGFXMaterialLayer>> TreeSelectedLayers);

	bool IsDetailsPropertyVisible(const FPropertyAndParent& PropertyAndParent);

	bool IsDetailsRowVisible(FName InRowName, FName InParentName);

	/** Called whenever a layer is added, removed, or reparented. */
	void OnLayersChanged();

	/** Called when a layer is added, removed, or reparented by the layers view. */
	void OnLayersChangedByLayersView();

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FMGFXMaterialEditor"); }

	// FNotifyHook
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged) override;

	/** Set a scalar parameter value, either on the preview material if interactive, or directly on the generated material expressions. */
	void SetMaterialScalarParameterValue(FName PropertyName, float Value, bool bInteractive) const;

	/** Set a vector parameter value, either on the preview material if interactive, or directly on the generated material expressions. */
	void SetMaterialVectorParameterValue(FName PropertyName, FLinearColor Value, bool bInteractive) const;

	// FEditorUndoClient
	virtual bool MatchesContext(const FTransactionContext& InContext,
	                            const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjectContexts) const override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	DECLARE_MULTICAST_DELEGATE_OneParam(FMaterialChangedDelegate, UMaterialInterface* /*NewMaterial*/);

	/** Called when the material asset has been set or changed. */
	FMaterialChangedDelegate OnMaterialChangedEvent;

	/** Called when the preview material instance has been set or changed. */
	FMaterialChangedDelegate OnPreviewMaterialChangedEvent;

	DECLARE_MULTICAST_DELEGATE_OneParam(FLayerSelectionChangedDelegate, const TArray<TObjectPtr<UMGFXMaterialLayer>>& /*SelectedLayers*/);

	/** Called when the layer selection has changed. */
	FLayerSelectionChangedDelegate OnLayerSelectionChangedEvent;

	DECLARE_MULTICAST_DELEGATE(FLayersChangedDelegate);

	/** Called when a layer is added, removed, or reparented. */
	FLayersChangedDelegate OnLayersChangedEvent;

private:
	TSharedPtr<SMGFXMaterialEditorCanvas> CanvasWidget;

	TSharedPtr<SMGFXMaterialEditorLayers> LayersWidget;

	TSharedPtr<IDetailsView> DetailsView;

	/** The generator used to build a UMaterial from a UMGFXMaterial. */
	TSharedPtr<FMGFXMaterialGenerator> Generator;

	/** The MGFX material asset being edited. */
	TObjectPtr<UMGFXMaterial> MGFXMaterial;

	/** The dynamic preview material to display in the editor. */
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;

	/** The currently selected layers. */
	TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedLayers;

	/** Automatically regenerate the material after every edit. */
	bool bAutoRegenerate;

	void BindCommands();
	void RegisterToolbar();

	TSharedRef<SDockTab> SpawnTab_Canvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Layers(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);

	/** Called when the material asset of the MGFX material has changed to a new asset. */
	void OnMaterialAssetChanged();

	/** Create or recreate the preview material instance dynamic. */
	void UpdatePreviewMaterial();

	/** Find an open material editor for a material. */
	static IMaterialEditor* FindMaterialEditor(UMaterial* Material);

	/** Return the duplicated preview material that a material editor is displaying. */
	static UPreviewMaterial* GetMaterialEditorPreviewMaterial(IMaterialEditor* MaterialEditor);

	void DeleteSelectedLayers();
	bool CanDeleteSelectedLayers();
	void CopySelectedLayers();
	bool CanCopySelectedLayers();
	void CutSelectedLayers();
	bool CanCutSelectedLayers();
	void PasteLayers();
	bool CanPasteLayers();
	void DuplicateSelectedLayers();
	bool CanDuplicateSelectedLayers();

public:
	static const FName CanvasTabId;
	static const FName LayersTabId;
	static const FName DetailsTabId;
};
