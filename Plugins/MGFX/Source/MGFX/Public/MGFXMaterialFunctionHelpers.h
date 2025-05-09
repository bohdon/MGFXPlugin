﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class UMaterialFunctionInterface;


/**
 * Helper functions for retrieving material functions from the plugin.
 */
class MGFX_API FMGFXMaterialFunctions
{
public:
	/**
	 * Return a soft object pointer to a material function in the plugin.
	 * Expects a relative path with no object name, e.g. "MySubDir/MyFunctionName".
	 */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetFunction(const FString RelativePath);

	/** Return a soft object pointer to a material function in the Merge folder. */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetMerge(const FString& Name);

	/** Return a soft object pointer to a material function in the Shape folder. */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetShape(const FString& Name);

	/** Return a soft object pointer to a material function in the Transform folder. */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetTransform(const FString& Name);

	/** Return a soft object pointer to a material function in the Visual folder */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetVisual(const FString& Name);

	/** Return a soft object pointer to a material function in the Util folder. */
	static TSoftObjectPtr<UMaterialFunctionInterface> GetUtil(const FString& Name);

public:
	/** The unreal path to all material functions in the plugin. */
	static FString MaterialFunctionsPath;

	static FString MergePath;
	static FString ShapePath;
	static FString TransformPath;
	static FString UtilPath;
	static FString VisualPath;
};
