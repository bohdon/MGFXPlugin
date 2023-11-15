// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMGFXMaterialEditor.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialBuilder.h"

class IMaterialEditor;
class UMGFXMaterial;
class UMGFXMaterialShapeFill;
class UMGFXMaterialShapeStroke;
class UMaterialExpressionNamedRerouteDeclaration;


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
	void Generate_DeleteAllNodes(FMGFXMaterialBuilder& Builder);

	/** Add a generated warning comment to prevent user modification. */
	void Generate_AddWarningComment(FMGFXMaterialBuilder& Builder);

	/** Create boilerplate UVs based on desired canvas size. */
	void Generate_AddUVsBoilerplate(FMGFXMaterialBuilder& Builder);

	/** Generate all shape layers and combine them. */
	void Generate_Shapes(FMGFXMaterialBuilder& Builder);

	/** Generate material nodes to apply a 2D transform. */
	UMaterialExpression* Generate_TransformUVs(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                           const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
	                                           const FString& ParamPrefix, const FName& ParamGroup, bool bCreateReroute = true);

	/** Generate material nodes to create a shape. */
	UMaterialExpression* Generate_Shape(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
	                                    UMaterialExpression* InUVsExp, const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes to create the visuals for a shape.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* Generate_ShapeVisuals(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShape* Shape,
	                                           UMaterialExpression* ShapeExp, const FString& ParamPrefix, const FName& ParamGroup);

	/** Generate material nodes to merge two shape layers. */
	UMaterialExpression* Generate_MergeShapes(FMGFXMaterialBuilder& Builder, FVector2D& NodePos,
	                                          UMaterialExpression* ShapeAExp, UMaterialExpression* ShapeBExp,
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
	                                        UMaterialExpression* ShapeExp, const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes for a shape stroke.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* Generate_ShapeStroke(FMGFXMaterialBuilder& Builder, FVector2D& NodePos, const UMGFXMaterialShapeStroke* Stroke,
	                                          UMaterialExpression* ShapeExp, const FString& ParamPrefix, const FName& ParamGroup);

public:
	static const FName DetailsTabId;
	static const FName CanvasTabId;
	static const FName Reroute_CanvasUVs;
	static const FName Reroute_FilterWidth;
	static const FName Reroute_ShapesOutput;
	static const int32 GridSize;
};
