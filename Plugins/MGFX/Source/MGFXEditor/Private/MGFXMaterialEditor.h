// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMGFXMaterialEditor.h"
#include "MGFXMaterialTypes.h"
#include "MGFXMaterialBuilder.h"

class IMGFXMaterialLayerParentInterface;
class IMaterialEditor;
class SArtboardPanel;
class SImage;
class SMGFXMaterialEditorCanvas;
class SMGFXMaterialEditorLayers;
class UMGFXMaterial;
class UMGFXMaterialLayer;
class UMGFXMaterialShape;
class UMGFXMaterialShapeFill;
class UMGFXMaterialShapeStroke;
class UMaterialExpressionNamedRerouteDeclaration;


/**
 * Represents a paired set of UVs and matching calculated FilterWidth.
 */
struct FMGFXMaterialUVsAndFilterWidth
{
	/** The uvs expression. */
	UMaterialExpressionNamedRerouteDeclaration* UVsExp = nullptr;

	/** The calculated filter width for the UVs. */
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp = nullptr;
};


/**
 * Tracks output expressions for a single layer in a generated material.
 */
struct FMGFXMaterialLayerOutputs
{
	/** The UVs and calculated filter width of this layer. */
	FMGFXMaterialUVsAndFilterWidth UVs;

	/** The shape sdf expression for the layer. */
	UMaterialExpression* ShapeExp = nullptr;

	/** The merged result of all visuals for the layer. */
	UMaterialExpression* VisualExp = nullptr;
};


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

	virtual ~FMGFXMaterialEditor();


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

	/** The MGFX material asset being edited. */
	TObjectPtr<UMGFXMaterial> MGFXMaterial;

	/** The dynamic preview material to display in the editor. */
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;

	/** The currently selected layers. */
	TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedLayers;

	/** Leftmost position where nodes for each layer should be aligned in the generated material. */
	float NodePosBaselineLeft;

	/** Reroute color to use for SDF shapes. */
	FLinearColor SDFRerouteColor;

	/** Reroute color to use for RGBA channels. */
	FLinearColor RGBARerouteColor;

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

	/** Delete all non-root nodes from a material graph. */
	void Generate_DeleteAllNodes(FMGFXMaterialBuilder& Builder);

	/** Add a generated warning comment to prevent user modification. */
	void Generate_AddWarningComment(FMGFXMaterialBuilder& Builder);

	/** Create boilerplate UVs based on desired canvas size. */
	void Generate_AddUVsBoilerplate(FMGFXMaterialBuilder& Builder);

	/** Generate all layers and combine them. */
	void Generate_Layers(FMGFXMaterialBuilder& Builder);

	/** Generate a layer and all it's children recursively. */
	FMGFXMaterialLayerOutputs Generate_Layer(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialLayer* Layer,
	                                         const FMGFXMaterialUVsAndFilterWidth& UVs, const FMGFXMaterialLayerOutputs& PrevOutputs);

	/** Generate material nodes to apply a 2D transform. */
	UMaterialExpression* Generate_TransformUVs(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                           const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
	                                           const FString& ParamPrefix, const FName& ParamGroup);

	/** Generate material nodes to create a shape. */
	UMaterialExpression* Generate_Shape(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
	                                    UMaterialExpression* InUVsExp, const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes to create the visuals for a shape.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* Generate_ShapeVisuals(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
	                                           UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                           const FString& ParamPrefix, const FName& ParamGroup);

	/** Merge two visual (RGBA) layers. */
	UMaterialExpression* Generate_MergeVisual(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                          UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXLayerMergeOperation Operation,
	                                          const FString& ParamPrefix, const FName& ParamGroup);

	/** Merge two shape (SDF) layers. */
	UMaterialExpression* Generate_MergeShapes(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                          UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXShapeMergeOperation Operation,
	                                          const FString& ParamPrefix, const FName& ParamGroup);


	/** Generate material nodes for a vector 2 parameter using two scalars. */
	UMaterialExpression* Generate_Vector2Parameter(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                               FVector2f DefaultValue, const FString& ParamPrefix, const FName& ParamGroup,
	                                               int32 BaseSortPriority, const FString& ParamNameX, const FString& ParamNameY);

	/**
	 * Generate material nodes for a shape fill.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* Generate_ShapeFill(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShapeFill* Fill,
	                                        UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                        const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes for a shape stroke.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* Generate_ShapeStroke(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShapeStroke* Stroke,
	                                          UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                          const FString& ParamPrefix, const FName& ParamGroup);

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
	static const FName Reroute_CanvasUVs;
	static const FName Reroute_CanvasFilterWidth;
	static const FName Reroute_LayersOutput;
	static const int32 GridSize;
	static const FString Param_LocationX;
	static const FString Param_LocationY;
	static const FString Param_Rotation;
	static const FString Param_ScaleX;
	static const FString Param_ScaleY;
};
