﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneTypes.h"
#include "UObject/SoftObjectPtr.h"

class UMaterialFunctionInterface;
class UMaterialExpressionMaterialFunctionCall;
class UMaterialExpressionComment;
class UMaterial;
class UMaterialExpression;
class UMaterialExpressionAppendVector;
class UMaterialExpressionComponentMask;
class UMaterialExpressionNamedRerouteDeclaration;
class UMaterialExpressionNamedRerouteUsage;
class UMaterialExpressionParameter;
class UMaterialExpressionScalarParameter;


/**
 * Utility class for building materials programmatically.
 */
struct MGFXEDITOR_API FMGFXMaterialBuilder
{
	FMGFXMaterialBuilder();

	FMGFXMaterialBuilder(UMaterial* InMaterial, bool bInBlockPostEditChange);

	~FMGFXMaterialBuilder();

	TObjectPtr<UMaterial> GetMaterial() const { return Material; }

	/**
	 * Set the material that will be modified.
	 * If bInBlockPostEditChange is true, temporarily make the material a preview material, so that
	 * If bInBlockPostEditChange is true, temporarily make the material a preview material, so that
	 * material expression updates don't trigger PostEditChange on the material itself.
	 */
	void SetMaterial(UMaterial* InMaterial, bool bInBlockPostEditChange);

	/**
	 * Reset the builder, clearing the material reference and restoring its preview state.
	 * Should be called when finished building.
	 */
	void Reset();

	/** Create a new material expression. */
	UMaterialExpression* Create(TSubclassOf<UMaterialExpression> ExpressionClass, const FVector2D& NodePos) const;

	/** Templated create a new material expression. */
	template <class T>
	T* Create(const FVector2D& NodePos) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, UMaterialExpression>::Value, "'T' template parameter to Create must be derived from UMaterialExpression");

		return CastChecked<T>(Create(T::StaticClass(), NodePos));
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

	UMaterialExpressionParameter* FindNamedParameter(const FName ParameterName) const;

	template <class T>
	T* FindNamedParameter(const FName ParameterName) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, UMaterialExpressionParameter>::Value,
		              "'T' template parameter to FindNamedParameter must be derived from UMaterialExpressionParameter");

		return Cast<T>(FindNamedParameter(ParameterName));
	}

	/** Set the value of an existing scalar parameter expression by name. */
	void SetScalarParameterValue(FName ParameterName, float Value);

	/** Set the value of an existing vector parameter expression by name. */
	void SetVectorParameterValue(FName ParameterName, FLinearColor Value);

	/** Connect two expressions using explicit pin names. */
	bool Connect(UMaterialExpression* From, const FString& FromPin, UMaterialExpression* To, const FString& ToPin) const;

	/** Connect the first output of an expression to the first input of another. */
	bool Connect(UMaterialExpression* From, UMaterialExpression* To) const;

	bool ConnectProperty(UMaterialExpression* From, const FString& FromPin, const EMaterialProperty& Property) const;

	/** Set the name, group, and sort priority of a parameter. */
	void ConfigureParameter(UMaterialExpressionParameter* ParameterExp, FName ParameterName, FName Group, int32 SortPriority = 32) const;

	void RecompileMaterial();

	/** Delete all expressions in the material. */
	void DeleteAll();

protected:
	/** When true, prevent the material from being recompiled in post edit change property calls. */
	bool bBlockPostEditChange = false;

	/** The material being edited. */
	TObjectPtr<UMaterial> Material = nullptr;

	/** The original value of bIsPreviewMaterial for the material before it was modified. */
	bool bWasPreviewMaterial = false;
};
