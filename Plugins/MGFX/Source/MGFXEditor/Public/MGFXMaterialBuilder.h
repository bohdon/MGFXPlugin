// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

class UMaterialExpressionScalarParameter;
class IMaterialEditor;
class UMaterialExpressionAppendVector;
class UMaterialExpressionComponentMask;
class UMaterialExpressionNamedRerouteDeclaration;
class UMaterialExpressionNamedRerouteUsage;
class UMaterialExpressionParameter;
class UMaterialGraph;


/**
 * Utility class for building materials programmatically.
 */
struct MGFXEDITOR_API FMGFXMaterialBuilder
{
	FMGFXMaterialBuilder(IMaterialEditor* InMaterialEditor, UMaterialGraph* InMaterialGraph);

	/** The material editor being used to author the material. */
	TObjectPtr<IMaterialEditor> MaterialEditor;

	/** The material graph that owns the preview material being edited. */
	TObjectPtr<UMaterialGraph> MaterialGraph;

	/** Create a new material expression. */
	UMaterialExpression* Create(TSubclassOf<UMaterialExpression> ExpressionClass, const FVector2D& NodePos) const;

	/** Templated create a new material expression. */
	template <class T>
	T* Create(const FVector2D& NodePos) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, UMaterialExpression>::Value, "'T' template parameter to Create must be derived from UMaterialExpression");

		return Cast<T>(Create(T::StaticClass(), NodePos));
	}

	UMaterialExpressionComment* CreateComment(const FVector2D& NodePos, const FString& Text, FLinearColor Color = FLinearColor(0.02f, 0.02f, 0.02f)) const;

	/** Create a material function expression. */
	UMaterialExpressionMaterialFunctionCall* CreateFunction(const FVector2D& NodePos, UMaterialFunctionInterface* Function) const;

	/** Create a material function expression. */
	UMaterialExpressionMaterialFunctionCall* CreateFunction(const FVector2D& NodePos, const TSoftObjectPtr<UMaterialFunctionInterface>& FunctionPtr) const;

	/** Create a component mask expression with the specified channels, 0 or 1. */
	UMaterialExpressionComponentMask* CreateComponentMask(const FVector2D& NodePos, uint32 R, uint32 G, uint32 B, uint32 A) const;

	/** Create a component mask expression with the RGB channels. */
	UMaterialExpressionComponentMask* CreateComponentMaskRGB(const FVector2D& NodePos) const;

	/** Create a component mask expression with only the A channel. */
	UMaterialExpressionComponentMask* CreateComponentMaskA(const FVector2D& NodePos) const;

	/** Create and connect an append expression. */
	UMaterialExpressionAppendVector* CreateAppend(const FVector2D& NodePos, UMaterialExpression* A, UMaterialExpression* B) const;

	/** Create and configure a scalar parameter expression. */
	UMaterialExpressionScalarParameter* CreateScalarParam(const FVector2D& NodePos, FName ParameterName, FName Group, int32 SortPriority = 32) const;

	/** Create a named reroute declaration, and connect it to an output. */
	UMaterialExpressionNamedRerouteDeclaration* CreateNamedReroute(const FVector2D& NodePos, FName Name) const;

	UMaterialExpressionNamedRerouteDeclaration* CreateNamedReroute(const FVector2D& NodePos, FName Name, FLinearColor Color) const;

	/** Create a usage of an existing named reroute. */
	UMaterialExpressionNamedRerouteUsage* CreateNamedRerouteUsage(const FVector2D& NodePos, UMaterialExpressionNamedRerouteDeclaration* Declaration) const;

	/** Find and create a usage of an existing named reroute. */
	UMaterialExpressionNamedRerouteUsage* CreateNamedRerouteUsage(const FVector2D& NodePos, const FName Name) const;

	/** Find a named reroute. */
	UMaterialExpressionNamedRerouteDeclaration* FindNamedReroute(const FName Name) const;

	/** Connect two expressions using explicit pin names. */
	bool Connect(UMaterialExpression* From, const FString& FromPin, UMaterialExpression* To, const FString& ToPin) const;

	/** Connect the first output of an expression to the first input of another. */
	bool Connect(UMaterialExpression* From, UMaterialExpression* To) const;

	bool ConnectProperty(UMaterialExpression* From, const FString& FromPin, const EMaterialProperty& Property) const;

	/** Set the name, group, and sort priority of a parameter. */
	void ConfigureParameter(UMaterialExpressionParameter* ParameterExp, FName ParameterName, FName Group, int32 SortPriority = 32) const;
};
