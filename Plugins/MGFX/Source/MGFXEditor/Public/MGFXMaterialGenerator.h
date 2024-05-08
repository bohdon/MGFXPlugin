// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGFXMaterialBuilder.h"
#include "MGFXMaterialTypes.h"

class UMGFXMaterial;
class UMGFXMaterialLayer;
class UMGFXMaterialShape;
class UMGFXMaterialShapeFill;
class UMGFXMaterialShapeStroke;
class UMaterialExpression;
class UMaterialExpressionNamedRerouteDeclaration;


/**
 * Represents a paired set of UVs and matching calculated FilterWidth.
 */
struct MGFXEDITOR_API FMGFXMaterialUVsAndFilterWidth
{
	/** The uvs expression. */
	UMaterialExpressionNamedRerouteDeclaration* UVsExp = nullptr;

	/** The calculated filter width for the UVs. */
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp = nullptr;
};


/**
 * Tracks output expressions for a single layer in a generated material.
 */
struct MGFXEDITOR_API FMGFXMaterialLayerOutputs
{
	/** The UVs and calculated filter width of this layer. */
	FMGFXMaterialUVsAndFilterWidth UVs;

	/** The shape sdf expression for the layer. */
	UMaterialExpression* ShapeExp = nullptr;

	/** The merged result of all visuals for the layer. */
	UMaterialExpression* VisualExp = nullptr;
};


/**
 * Handles generating a UMaterial from a UMGFXMaterial
 */
class MGFXEDITOR_API FMGFXMaterialGenerator
{
public:
	FMGFXMaterialGenerator();

	/** Reroute color to use for SDF shapes. */
	FLinearColor SDFRerouteColor = FLinearColor(0.f, 0.f, 0.f);

	/** Reroute color to use for RGBA channels. */
	FLinearColor RGBARerouteColor = FLinearColor(0.22f, .09f, 0.55f);

	FLinearColor CommentColor = FLinearColor(0.06f, 0.02f, 0.02f);

	/** Generate the material. */
	void Generate(UMGFXMaterial* InMGFXMaterial, UMaterial* OutputMaterial);

	/** Add a generated warning comment to prevent user modification. */
	void AddWarningComment();

	/** Create boilerplate UVs based on desired canvas size. */
	void AddUVsBoilerplate();

	/** Generate all layers and combine them. */
	void GenerateLayers();

	/** Generate a layer and all it's children recursively. */
	FMGFXMaterialLayerOutputs GenerateLayer(const UMGFXMaterialLayer* Layer,
	                                        const FMGFXMaterialUVsAndFilterWidth& UVs, const FMGFXMaterialLayerOutputs& PrevOutputs);

	/** Generate material nodes to apply a 2D transform. */
	UMaterialExpression* GenerateTransformUVs(const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
	                                          const FString& ParamPrefix, const FName& ParamGroup);

	/** Generate material nodes to create a shape. */
	UMaterialExpression* GenerateShape(const UMGFXMaterialShape* Shape,
	                                   UMaterialExpression* InUVsExp, const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes to create the visuals for a shape.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* GenerateShapeVisuals(const UMGFXMaterialShape* Shape,
	                                          UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                          const FString& ParamPrefix, const FName& ParamGroup);

	/** Merge two visual (RGBA) layers. */
	UMaterialExpression* GenerateMergeVisual(UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXLayerMergeOperation Operation,
	                                         const FString& ParamPrefix, const FName& ParamGroup);

	/** Merge two shape (SDF) layers. */
	UMaterialExpression* GenerateMergeShapes(UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXShapeMergeOperation Operation,
	                                         const FString& ParamPrefix, const FName& ParamGroup);


	/** Generate material nodes for a vector 2 parameter using two scalars. */
	UMaterialExpression* GenerateVector2Parameter(FVector2f DefaultValue, const FString& ParamPrefix, const FName& ParamGroup,
	                                              int32 BaseSortPriority, const FString& ParamNameX, const FString& ParamNameY);

	/**
	 * Generate material nodes for a shape fill.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* GenerateShapeFill(const UMGFXMaterialShapeFill* Fill,
	                                       UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                       const FString& ParamPrefix, const FName& ParamGroup);

	/**
	 * Generate material nodes for a shape stroke.
	 * Returns an unpremultiplied 4-channel RGBA expression.
	 */
	UMaterialExpression* GenerateShapeStroke(const UMGFXMaterialShapeStroke* Stroke,
	                                         UMaterialExpression* ShapeExp, UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
	                                         const FString& ParamPrefix, const FName& ParamGroup);

protected:
	/** The MGFXMaterial that is being used to generate a material. */
	TObjectPtr<UMGFXMaterial> MGFXMaterial = nullptr;

	/** The builder use to author the material. */
	FMGFXMaterialBuilder Builder;

	/** The current position to use for new nodes. */
	FVector2D Pos;

public:
	int32 GridSize;

	/** Leftmost position where nodes for each layer should be aligned in the generated material. */
	float NodePosBaselineLeft;

	FName Reroute_CanvasUVs;
	FName Reroute_CanvasFilterWidth;
	FName Reroute_LayersOutput;

	FString Param_LocationX;
	FString Param_LocationY;
	FString Param_Rotation;
	FString Param_ScaleX;
	FString Param_ScaleY;
};
