﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialGenerator.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialFunctionHelpers.h"
#include "MGFXPropertyMacros.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Shapes/MGFXMaterialShape.h"
#include "Shapes/MGFXMaterialShapeVisual.h"


FMGFXMaterialGenerator::FMGFXMaterialGenerator()
	: NodePosBaselineLeft(GridSize * -64),
	  Reroute_CanvasUVs(TEXT("CanvasUVs")),
	  Reroute_CanvasFilterWidth(TEXT("CanvasFilterWidth")),
	  Reroute_LayersOutput(TEXT("LayersOutput")),
	  Param_LocationX(TEXT("LocationX")),
	  Param_LocationY(TEXT("LocationY")),
	  Param_Rotation(TEXT("Rotation")),
	  Param_ScaleX(TEXT("ScaleX")),
	  Param_ScaleY(TEXT("ScaleY"))
{
}

void FMGFXMaterialGenerator::Generate(UMGFXMaterial* InMGFXMaterial, UMaterial* OutputMaterial, bool bRecompile, bool bInIsPreviewMaterial)
{
	SCOPED_NAMED_EVENT(FMGFXMaterialGenerator_Generate, FColor::Green);

	check(InMGFXMaterial);
	check(OutputMaterial);

	MGFXMaterial = InMGFXMaterial;
	bIsPreviewMaterial = bInIsPreviewMaterial;
	Builder.SetMaterial(OutputMaterial, true);
	Pos = FVector2D::ZeroVector;

	OutputMaterial->MaterialDomain = MGFXMaterial->MaterialDomain;
	OutputMaterial->BlendMode = MGFXMaterial->BlendMode;

	Builder.DeleteAll();

	AddWarningComment();
	AddUVsBoilerplate();

	GenerateLayers();

	// build final output
	Pos = FVector2D(GridSize * -31, 0);

	if (MGFXMaterial->OutputProperty == MP_EmissiveColor)
	{
		// connect layer output to final color
		UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(Pos, Reroute_LayersOutput);
		Builder.ConnectProperty(OutputUsageExp, "", MGFXMaterial->OutputProperty);

		Pos += GridSize * FVector2D(16, 8);

		// connect opacity output
		UMaterialExpressionComponentMask* AlphaOutMaskExp = Builder.CreateComponentMaskA(Pos);
		Builder.Connect(OutputUsageExp, AlphaOutMaskExp);
		Builder.ConnectProperty(AlphaOutMaskExp, "", MP_Opacity);
	}
	else
	{
		// connect constant to final color
		UMaterialExpressionConstant3Vector* ColorExp = Builder.Create<UMaterialExpressionConstant3Vector>(Pos);
		SET_PROP(ColorExp, Constant, MGFXMaterial->DefaultEmissiveColor);
		Builder.ConnectProperty(ColorExp, "", MP_EmissiveColor);

		Pos.Y += GridSize * 15;

		// connect layer output to custom property
		UMaterialExpressionNamedRerouteUsage* OutputUsageExp = Builder.CreateNamedRerouteUsage(Pos, Reroute_LayersOutput);
		Builder.ConnectProperty(OutputUsageExp, "", MGFXMaterial->OutputProperty);
	}

	if (bRecompile)
	{
		SCOPED_NAMED_EVENT(FMGFXMaterialGenerator_Generate_Recompile, FColor::Green);
		Builder.RecompileMaterial();
	}

	// clear material references when finished
	MGFXMaterial = nullptr;
	Builder.Reset();
}

void FMGFXMaterialGenerator::AddWarningComment()
{
	const FString Text = FString::Printf(TEXT("Generated by %s\nDo not edit manually"), *GetNameSafe(MGFXMaterial));

	Pos = FVector2D(GridSize * -20, GridSize * -10);
	UMaterialExpressionComment* CommentExp = Builder.CreateComment(Pos, Text, CommentColor);
}

void FMGFXMaterialGenerator::AddUVsBoilerplate()
{
	Pos = FVector2D(GridSize * -64, GridSize * 30);

	// create CanvasWidth and CanvasHeight scalar parameters
	const FName ParamGroup("Canvas");
	auto* CanvasAppendExp = GenerateVector2Parameter(MGFXMaterial->BaseCanvasSize,
	                                                 FString(), ParamGroup, 0, "CanvasWidth", "CanvasHeight");

	Pos.X += GridSize * 15;

	// create canvas tex cords with default UVs (for UI these will be 9-sliced)
	UMaterialExpressionTextureCoordinate* TexCoordExp = Builder.Create<UMaterialExpressionTextureCoordinate>(Pos + FVector2D(0, GridSize * 8));

	Pos.X += GridSize * 15;

	// scale UVs to canvas size so all shapes can operate in canvas-dimensions
	UMaterialExpressionMultiply* MultiplyExp = Builder.Create<UMaterialExpressionMultiply>(Pos);
	Builder.Connect(CanvasAppendExp, "", MultiplyExp, "A");
	Builder.Connect(TexCoordExp, "", MultiplyExp, "B");

	Pos.X += GridSize * 15;

	// output to canvas UVs reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = Builder.CreateNamedReroute(Pos, Reroute_CanvasUVs);
	Builder.Connect(MultiplyExp, OutputRerouteExp);

	Pos.X += GridSize * 15;

	// compute a reusable filter width based on canvas resolution
	UMaterialExpression* FilterWidthExp;

	if (MGFXMaterial->bComputeFilterWidth)
	{
		// create filter width func
		UMaterialExpressionMaterialFunctionCall* FilterWidthFuncExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetVisual("FilterWidth"));
		Builder.Connect(OutputRerouteExp, FilterWidthFuncExp);

		Pos.X += GridSize * 15;

		FilterWidthExp = FilterWidthFuncExp;
	}
	else
	{
		UMaterialExpressionConstant* FilterWidthConstExp = Builder.Create<UMaterialExpressionConstant>(Pos);
		SET_PROP(FilterWidthConstExp, R, MGFXMaterial->FixedFilterWidth);

		Pos.X += GridSize * 15;

		FilterWidthExp = FilterWidthConstExp;
	}

	// add filter width reroute
	UMaterialExpressionNamedRerouteDeclaration* FilterWidthRerouteExp = Builder.CreateNamedReroute(Pos, Reroute_CanvasFilterWidth);
	Builder.Connect(FilterWidthExp, FilterWidthRerouteExp);
}

void FMGFXMaterialGenerator::GenerateLayers()
{
	Pos = FVector2D(NodePosBaselineLeft, GridSize * 80);

	// create base UVs
	UMaterialExpressionNamedRerouteDeclaration* CanvasUVsDeclaration = Builder.FindNamedReroute(Reroute_CanvasUVs);
	UMaterialExpressionNamedRerouteDeclaration* CanvasFilterWidthDeclaration = Builder.FindNamedReroute(Reroute_CanvasFilterWidth);
	check(CanvasUVsDeclaration);
	check(CanvasFilterWidthDeclaration);

	const FMGFXMaterialUVsAndFilterWidth CanvasUVs(CanvasUVsDeclaration, CanvasFilterWidthDeclaration);

	// generate all layers recursively
	FMGFXMaterialLayerOutputs LayerOutputs = FMGFXMaterialLayerOutputs();
	for (int32 Idx = MGFXMaterial->RootLayers.Num() - 1; Idx >= 0; --Idx)
	{
		const UMGFXMaterialLayer* ChildLayer = MGFXMaterial->RootLayers[Idx];

		LayerOutputs = GenerateLayer(ChildLayer, CanvasUVs, LayerOutputs);
	}

	Pos.X += GridSize * 15;

	// unpremult after all merges
	UMaterialExpressionMaterialFunctionCall* UnpremultExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetUtil("Unpremult"));
	Builder.Connect(LayerOutputs.VisualExp, UnpremultExp);

	Pos.X += GridSize * 15;

	// connect to layers output reroute
	UMaterialExpressionNamedRerouteDeclaration* OutputRerouteExp = Builder.CreateNamedReroute(Pos, Reroute_LayersOutput, RGBARerouteColor);
	Builder.Connect(UnpremultExp, OutputRerouteExp);
}

FMGFXMaterialLayerOutputs FMGFXMaterialGenerator::GenerateLayer(const UMGFXMaterialLayer* Layer,
                                                                const FMGFXMaterialUVsAndFilterWidth& UVs, const FMGFXMaterialLayerOutputs& PrevOutputs)
{
	// collect expressions for this layers output
	FMGFXMaterialLayerOutputs LayerOutputs;

	const FString LayerName = Layer->Name.IsEmpty() ? Layer->GetName() : Layer->Name;
	const FString ParamPrefix = LayerName + ".";
	const FName ParamGroup = FName(FString::Printf(TEXT("%s"), *LayerName));

	// reset baseline and move to new line
	Pos.X = NodePosBaselineLeft;
	Pos.Y += GridSize * 40;


	// generate transform uvs
	UMaterialExpressionNamedRerouteUsage* ParentUVsUsageExp = Builder.CreateNamedRerouteUsage(Pos, UVs.UVsExp);

	Pos.X += GridSize * 15;

	// apply layer transform. this may just return the parent uvs if optimized out
	UMaterialExpression* UVsExp = GenerateTransformUVs(Layer->Transform, ParentUVsUsageExp, ParamPrefix, ParamGroup);

	// store as a reroute for any children
	const bool bCreateReroute = Layer->HasLayers();
	if (bCreateReroute)
	{
		// output to named reroute
		LayerOutputs.UVs.UVsExp = Builder.CreateNamedReroute(Pos, FName(ParamPrefix + "UVs"));
		Builder.Connect(UVsExp, "", LayerOutputs.UVs.UVsExp, "");
		// treat this as the uvs expression for layer
		UVsExp = LayerOutputs.UVs.UVsExp;

		Pos.X += GridSize * 15;
	}
	else
	{
		// reuse parent UVs
		LayerOutputs.UVs.UVsExp = UVs.UVsExp;
	}

	// recalculate filter width to use for these uvs if the SDF gradient can ever be scaled
	const bool bNoOptimization = Layer->Transform.bAnimatable || MGFXMaterial->bAllAnimatable || bIsPreviewMaterial;
	const bool bHasModifiedScale = !(Layer->Transform.Scale - FVector2f::One()).IsNearlyZero();
	if (MGFXMaterial->bComputeFilterWidth && (bNoOptimization || bHasModifiedScale))
	{
		UMaterialExpressionMaterialFunctionCall* UVFilterFuncExp = Builder.CreateFunction(
			Pos + FVector2D(0, GridSize * 8), FMGFXMaterialFunctions::GetVisual("FilterWidth"));

		Builder.Connect(UVsExp, "", UVFilterFuncExp, "");

		Pos.X += GridSize * 15;

		LayerOutputs.UVs.FilterWidthExp = Builder.CreateNamedReroute(Pos + FVector2D(0, GridSize * 8), FName(ParamPrefix + "FilterWidth"));
		Builder.Connect(UVFilterFuncExp, "", LayerOutputs.UVs.FilterWidthExp, "");

		Pos.X += GridSize * 15;
	}
	else
	{
		// reuse parent filter width
		LayerOutputs.UVs.FilterWidthExp = UVs.FilterWidthExp;
	}

	// generate children layers bottom to top
	FMGFXMaterialLayerOutputs ChildOutputs = LayerOutputs;
	for (int32 Idx = Layer->NumLayers() - 1; Idx >= 0; --Idx)
	{
		const UMGFXMaterialLayer* ChildLayer = Layer->GetLayer(Idx);

		ChildOutputs = GenerateLayer(ChildLayer, LayerOutputs.UVs, ChildOutputs);
	}


	// generate shape for this layer
	if (Layer->Shape)
	{
		// the shape, with an SDF output
		LayerOutputs.ShapeExp = GenerateShape(Layer->Shape, UVsExp, ParamPrefix, ParamGroup);

		// the shape visuals, including all fills and strokes
		LayerOutputs.VisualExp = GenerateShapeVisuals(Layer->Shape, LayerOutputs.ShapeExp,
		                                              LayerOutputs.UVs.FilterWidthExp, ParamPrefix, ParamGroup);
	}

	// merge this layer with its children
	if (Layer->HasLayers())
	{
		if (ChildOutputs.VisualExp && ChildOutputs.VisualExp != LayerOutputs.VisualExp)
		{
			if (LayerOutputs.VisualExp)
			{
				LayerOutputs.VisualExp = GenerateMergeVisual(LayerOutputs.VisualExp, ChildOutputs.VisualExp,
				                                             Layer->MergeOperation, ParamPrefix, ParamGroup);
			}
			else
			{
				// pass forward output from previous layer to ensure it makes it to the top
				LayerOutputs.VisualExp = ChildOutputs.VisualExp;
			}
		}
	}


	// merge this layer with the previous sibling
	if (PrevOutputs.ShapeExp && LayerOutputs.ShapeExp)
	{
		// TODO: this is weird, just setup explicit shape merging by layer reference, so the layer structure remains only for visuals
		LayerOutputs.ShapeExp = GenerateMergeShapes(PrevOutputs.ShapeExp, LayerOutputs.ShapeExp,
		                                            Layer->Shape->ShapeMergeOperation, ParamPrefix, ParamGroup);
	}

	if (PrevOutputs.VisualExp)
	{
		if (LayerOutputs.VisualExp)
		{
			LayerOutputs.VisualExp = GenerateMergeVisual(LayerOutputs.VisualExp, PrevOutputs.VisualExp,
			                                             Layer->MergeOperation, ParamPrefix, ParamGroup);
		}
		else
		{
			// pass forward output from previous layer to ensure it makes it to the top
			LayerOutputs.VisualExp = PrevOutputs.VisualExp;
		}
	}

	return LayerOutputs;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateTransformUVs(const FMGFXShapeTransform2D& Transform, UMaterialExpression* InUVsExp,
                                                                  const FString& ParamPrefix, const FName& ParamGroup)
{
	// points to the last expression from each operation, since some may be skipped due to optimization
	UMaterialExpression* LastInputExp = InUVsExp;

	// temporary offset used to simplify next node positioning
	FVector2D NodePosOffset = FVector2D::Zero();

	const bool bNoOptimization = Transform.bAnimatable || MGFXMaterial->bAllAnimatable || bIsPreviewMaterial;

	// apply translate
	if (bNoOptimization || !Transform.Location.IsZero())
	{
		Pos.Y += GridSize * 8;

		UMaterialExpression* TranslateValueExp = GenerateVector2Parameter(Transform.Location, ParamPrefix, ParamGroup,
		                                                                  10, Param_LocationX, Param_LocationY);
		Pos.Y -= GridSize * 8;

		// subtract so that coordinate space matches UMG, positive offset means going right or down
		UMaterialExpressionSubtract* TranslateUVsExp = Builder.Create<UMaterialExpressionSubtract>(Pos);
		Builder.Connect(LastInputExp, "", TranslateUVsExp, "A");
		Builder.Connect(TranslateValueExp, "", TranslateUVsExp, "B");
		LastInputExp = TranslateUVsExp;

		Pos.X += GridSize * 15;
	}

	// apply rotate
	if (bNoOptimization || !FMath::IsNearlyZero(Transform.Rotation))
	{
		NodePosOffset = FVector2D(0, GridSize * 8);
		UMaterialExpressionScalarParameter* RotationExp = Builder.CreateScalarParam(
			Pos + NodePosOffset, FName(ParamPrefix + Param_Rotation), ParamGroup, 20);
		SET_PROP(RotationExp, DefaultValue, Transform.Rotation);

		Pos.X += GridSize * 15;

		// TODO: perf: allow marking rotation as animatable or not and bake in the conversion when not needed
		// conversion from degrees, not baked in to support animation
		NodePosOffset = FVector2D(0, GridSize * 8);
		UMaterialExpressionDivide* RotateConvExp = Builder.Create<UMaterialExpressionDivide>(Pos + NodePosOffset);
		Builder.Connect(RotationExp, "", RotateConvExp, "");
		// negative to match UMG coordinate space, positive rotations are clockwise
		SET_PROP_R(RotateConvExp, ConstB, -360.f);

		Pos.X += GridSize * 15;

		UMaterialExpressionMaterialFunctionCall* RotateUVsExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetTransform("Rotate"));
		Builder.Connect(LastInputExp, "", RotateUVsExp, "UVs");
		Builder.Connect(RotateConvExp, "", RotateUVsExp, "Rotation");
		LastInputExp = RotateUVsExp;

		Pos.X += GridSize * 15;
	}

	// apply scale
	if (bNoOptimization || Transform.Scale != FVector2f::One())
	{
		Pos.Y += GridSize * 8;
		UMaterialExpression* ScaleValueExp = GenerateVector2Parameter(Transform.Scale,
		                                                              ParamPrefix, ParamGroup, 30, Param_ScaleX, Param_ScaleY);
		Pos.Y -= GridSize * 8;

		UMaterialExpressionDivide* ScaleUVsExp = Builder.Create<UMaterialExpressionDivide>(Pos);
		Builder.Connect(LastInputExp, "", ScaleUVsExp, "A");
		Builder.Connect(ScaleValueExp, "", ScaleUVsExp, "B");
		LastInputExp = ScaleUVsExp;

		Pos.X += GridSize * 15;
	}

	return LastInputExp;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateShape(const UMGFXMaterialShape* Shape, UMaterialExpression* InUVsExp,
                                                           const FString& ParamPrefix, const FName& ParamGroup)
{
	check(Shape);

	TArray<FMGFXMaterialShapeInput> Inputs = Shape->GetInputs();

	int32 ParamSortPriority = 50;

	// keep track of original Y so it can be restored after generating inputs
	const int32 OrigNodePoseY = Pos.Y;

	// create shape inputs
	TArray<UMaterialExpression*> InputExps;
	Pos.Y += GridSize * 8;
	for (const FMGFXMaterialShapeInput& Input : Inputs)
	{
		const FLinearColor Value = Input.Value;

		// TODO: allow user to mark as exposed
		bool bShouldBeParameter = true;

		if (bShouldBeParameter)
		{
			// setup input as scalar params, Vector2's will be represented by 2 scalar params
			UMaterialExpression* InputExp;
			switch (Input.Type)
			{
			default:
			case EMGFXMaterialShapeInputType::Float:
				InputExp = Builder.Create<UMaterialExpressionScalarParameter>(Pos);
				break;
			case EMGFXMaterialShapeInputType::Vector2:
				InputExp = GenerateVector2Parameter(FVector2f(Input.Value.R, Input.Value.G),
				                                    ParamPrefix, ParamGroup, ParamSortPriority, Input.Name + TEXT("X"), Input.Name + TEXT("Y"));
				++ParamSortPriority;
				break;
			case EMGFXMaterialShapeInputType::Vector3:
			case EMGFXMaterialShapeInputType::Vector4:
				// its normal to ignore the extra A channel for a Vector3 param
				InputExp = Builder.Create<UMaterialExpressionVectorParameter>(Pos);
				break;
			}
			check(InputExp);

			InputExps.Add(InputExp);

			// configure parameter
			// Vector2 params will already be configured, and the InputExp is actually an append.
			if (UMaterialExpressionParameter* ParamExp = Cast<UMaterialExpressionParameter>(InputExp))
			{
				// TODO: include Shape name in the prefix, since there may be multiple shapes
				Builder.ConfigureParameter(ParamExp, FName(ParamPrefix + Input.Name), ParamGroup, ParamSortPriority);
				++ParamSortPriority;
			}

			if (auto* ConstExp = Cast<UMaterialExpressionScalarParameter>(InputExp))
			{
				SET_PROP(ConstExp, DefaultValue, Value.R);
			}
			else if (auto* Const2Exp = Cast<UMaterialExpressionVectorParameter>(InputExp))
			{
				SET_PROP(Const2Exp, DefaultValue, Value);
			}

			Pos.Y += GridSize * 8;
		}
		else
		{
			// setup input as constant expressions
			TSubclassOf<UMaterialExpression> InputExpClass = nullptr;
			switch (Input.Type)
			{
			case EMGFXMaterialShapeInputType::Float:
				InputExpClass = UMaterialExpressionConstant::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector2:
				InputExpClass = UMaterialExpressionConstant2Vector::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector3:
				InputExpClass = UMaterialExpressionConstant3Vector::StaticClass();
				break;
			case EMGFXMaterialShapeInputType::Vector4:
				InputExpClass = UMaterialExpressionConstant4Vector::StaticClass();
				break;
			}
			check(InputExpClass);

			UMaterialExpression* InputExp = Builder.Create(InputExpClass, Pos);
			check(InputExp);
			InputExps.Add(InputExp);

			if (auto* ConstExp = Cast<UMaterialExpressionConstant>(InputExp))
			{
				SET_PROP(ConstExp, R, Value.R);
			}
			else if (auto* Const2Exp = Cast<UMaterialExpressionConstant2Vector>(InputExp))
			{
				SET_PROP(Const2Exp, R, Value.R);
				SET_PROP(Const2Exp, G, Value.G);
			}
			else if (auto* Const3Exp = Cast<UMaterialExpressionConstant3Vector>(InputExp))
			{
				SET_PROP(Const3Exp, Constant, Value);
			}
			else if (auto* Const4Exp = Cast<UMaterialExpressionConstant4Vector>(InputExp))
			{
				SET_PROP(Const4Exp, Constant, Value);
			}

			Pos.Y += GridSize * 8;
		}
	}
	check(Inputs.Num() == InputExps.Num());

	Pos.X += GridSize * 20;
	Pos.Y = OrigNodePoseY;

	// create shape function
	UMaterialExpressionMaterialFunctionCall* ShapeExp = Builder.CreateFunction(Pos, Shape->GetMaterialFunctionPtr());

	// connect uvs
	if (InUVsExp)
	{
		Builder.Connect(InUVsExp, "", ShapeExp, "UVs");
	}

	// connect inputs
	for (int32 Idx = 0; Idx < Inputs.Num(); ++Idx)
	{
		const FMGFXMaterialShapeInput& Input = Inputs[Idx];
		UMaterialExpression* InputExp = InputExps[Idx];

		Builder.Connect(InputExp, "", ShapeExp, Input.Name);
	}

	Pos.X += GridSize * 15;

	return ShapeExp;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateShapeVisuals(const UMGFXMaterialShape* Shape, UMaterialExpression* ShapeExp,
                                                                  UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                                  const FString& ParamPrefix, const FName& ParamGroup)
{
	// TODO: support multiple visuals
	// generate visuals
	for (const UMGFXMaterialShapeVisual* Visual : Shape->Visuals)
	{
		if (const UMGFXMaterialShapeFill* Fill = Cast<UMGFXMaterialShapeFill>(Visual))
		{
			return GenerateShapeFill(Fill, ShapeExp, FilterWidthExp, ParamPrefix, ParamGroup);
		}
		else if (const UMGFXMaterialShapeStroke* const Stroke = Cast<UMGFXMaterialShapeStroke>(Visual))
		{
			return GenerateShapeStroke(Stroke, ShapeExp, FilterWidthExp, ParamPrefix, ParamGroup);
		}
		// TODO: combine visuals, don't just use the last one
	}

	return nullptr;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateMergeVisual(UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXLayerMergeOperation Operation,
                                                                 const FString& ParamPrefix, const FName& ParamGroup)
{
	// default to over
	TSoftObjectPtr<UMaterialFunctionInterface> MergeFunc = nullptr;

	switch (Operation)
	{
	case EMGFXLayerMergeOperation::Over:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Over");
		break;
	case EMGFXLayerMergeOperation::Add:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Add");
		break;
	case EMGFXLayerMergeOperation::Subtract:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Subtract");
		break;
	case EMGFXLayerMergeOperation::Multiply:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Multiply");
		break;
	case EMGFXLayerMergeOperation::In:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("In");
		break;
	case EMGFXLayerMergeOperation::Out:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Out");
		break;
	case EMGFXLayerMergeOperation::Mask:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Mask");
		break;
	case EMGFXLayerMergeOperation::Stencil:
		MergeFunc = FMGFXMaterialFunctions::GetMerge("Stencil");
		break;
	default: ;
	}

	if (MergeFunc.IsNull())
	{
		// no valid merge function
		return AExp;
	}

	UMaterialExpressionMaterialFunctionCall* MergeExp = Builder.CreateFunction(Pos, MergeFunc);
	Builder.Connect(AExp, "", MergeExp, "A");
	Builder.Connect(BExp, "", MergeExp, "B");

	Pos.X += GridSize * 15;

	return MergeExp;
}


UMaterialExpression* FMGFXMaterialGenerator::GenerateMergeShapes(UMaterialExpression* AExp, UMaterialExpression* BExp, EMGFXShapeMergeOperation Operation,
                                                                 const FString& ParamPrefix, const FName& ParamGroup)
{
	if (Operation == EMGFXShapeMergeOperation::None)
	{
		return BExp;
	}

	return nullptr;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateVector2Parameter(FVector2f DefaultValue,
                                                                      const FString& ParamPrefix, const FName& ParamGroup, int32 BaseSortPriority,
                                                                      const FString& ParamNameX, const FString& ParamNameY)
{
	UMaterialExpressionScalarParameter* XExp = Builder.CreateScalarParam(
		Pos, FName(ParamPrefix + ParamNameX), ParamGroup, BaseSortPriority);
	SET_PROP(XExp, DefaultValue, DefaultValue.X);

	UMaterialExpressionScalarParameter* YExp = Builder.CreateScalarParam(
		Pos + FVector2D(0, GridSize * 6), FName(ParamPrefix + ParamNameY), ParamGroup, BaseSortPriority + 1);
	SET_PROP(YExp, DefaultValue, DefaultValue.Y);

	Pos.X += GridSize * 15;

	// append
	UMaterialExpressionAppendVector* AppendExp = Builder.CreateAppend(Pos, XExp, YExp);

	Pos.X += GridSize * 15;

	return AppendExp;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateShapeFill(const UMGFXMaterialShapeFill* Fill, UMaterialExpression* ShapeExp,
                                                               UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                               const FString& ParamPrefix, const FName& ParamGroup)
{
	// add reused filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthUsageExp = nullptr;
	if (!Fill->bComputeFilterWidth)
	{
		FilterWidthUsageExp = Builder.CreateNamedRerouteUsage(Pos + FVector2D(0, GridSize * 8), FilterWidthExp);
	}

	// optionally enable filter bias
	UMaterialExpressionStaticBool* EnableBiasExp = nullptr;
	if (Fill->bEnableFilterBias)
	{
		EnableBiasExp = Builder.Create<UMaterialExpressionStaticBool>(Pos + FVector2D(0, GridSize * 16));
		SET_PROP_R(EnableBiasExp, Value, 1);
	}

	Pos.X += GridSize * 15;

	// add fill func
	UMaterialExpressionMaterialFunctionCall* FillExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetVisual("Fill"));
	Builder.Connect(ShapeExp, "SDF", FillExp, "SDF");
	if (FilterWidthUsageExp)
	{
		Builder.Connect(FilterWidthUsageExp, "", FillExp, "FilterWidth");
	}
	if (EnableBiasExp)
	{
		Builder.Connect(EnableBiasExp, "", FillExp, "EnableFilterBias");
	}

	Pos.X += GridSize * 15;

	// add color param
	UMaterialExpressionVectorParameter* ColorExp = Builder.Create<UMaterialExpressionVectorParameter>(Pos + FVector2D(0, GridSize * 8));
	Builder.ConfigureParameter(ColorExp, FName(ParamPrefix + "Color"), ParamGroup, 40);
	SET_PROP_R(ColorExp, DefaultValue, Fill->GetColor());

	Pos.X += GridSize * 15;

	// mutiply by color, and append to RGBA
	UMaterialExpressionMaterialFunctionCall* TintExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetVisual("Tint"));
	Builder.Connect(FillExp, "", TintExp, "In");
	Builder.Connect(ColorExp, "", TintExp, "RGB");
	Builder.Connect(ColorExp, "A", TintExp, "A");

	Pos.X += GridSize * 15;

	return TintExp;
}

UMaterialExpression* FMGFXMaterialGenerator::GenerateShapeStroke(const UMGFXMaterialShapeStroke* Stroke, UMaterialExpression* ShapeExp,
                                                                 UMaterialExpressionNamedRerouteDeclaration* FilterWidthExp,
                                                                 const FString& ParamPrefix, const FName& ParamGroup)
{
	// add stroke width input
	UMaterialExpressionScalarParameter* StrokeWidthExp = Builder.CreateScalarParam(
		Pos + FVector2D(0, GridSize * 8), FName(ParamPrefix + "StrokeWidth"), ParamGroup, 40);
	SET_PROP(StrokeWidthExp, DefaultValue, Stroke->StrokeWidth);

	// add reused filter width input
	UMaterialExpressionNamedRerouteUsage* FilterWidthUsageExp = nullptr;
	if (!Stroke->bComputeFilterWidth)
	{
		FilterWidthUsageExp = Builder.CreateNamedRerouteUsage(Pos + FVector2D(0, GridSize * 14), FilterWidthExp);
	}

	Pos.X += GridSize * 15;

	// add stroke func
	UMaterialExpressionMaterialFunctionCall* StrokeExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetVisual("Stroke"));
	Builder.Connect(ShapeExp, "SDF", StrokeExp, "SDF");
	Builder.Connect(StrokeWidthExp, "", StrokeExp, "StrokeWidth");
	if (FilterWidthUsageExp)
	{
		Builder.Connect(FilterWidthUsageExp, "", StrokeExp, "FilterWidth");
	}

	Pos.X += GridSize * 15;

	// add color param
	UMaterialExpressionVectorParameter* ColorExp = Builder.Create<UMaterialExpressionVectorParameter>(Pos + FVector2D(0, GridSize * 8));
	Builder.ConfigureParameter(ColorExp, FName(ParamPrefix + "Color"), ParamGroup, 40);
	SET_PROP_R(ColorExp, DefaultValue, Stroke->GetColor());

	Pos.X += GridSize * 15;

	// mutiply by color, and append to RGBA
	UMaterialExpressionMaterialFunctionCall* TintExp = Builder.CreateFunction(Pos, FMGFXMaterialFunctions::GetVisual("Tint"));
	Builder.Connect(StrokeExp, "", TintExp, "In");
	Builder.Connect(ColorExp, "", TintExp, "RGB");
	Builder.Connect(ColorExp, "A", TintExp, "A");

	Pos.X += GridSize * 15;

	return TintExp;
}
