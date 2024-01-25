// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialFunctionHelpers.h"

#include "Misc/PackageName.h"
#include "Misc/PathViews.h"


FString FMGFXMaterialFunctions::MaterialFunctionsPath(TEXT("/MGFX/MaterialFunctions/"));
FString FMGFXMaterialFunctions::MergePath(TEXT("Merge/MF_MGFX_Merge_"));
FString FMGFXMaterialFunctions::ShapePath(TEXT("Shape/MF_MGFX_Shape_"));
FString FMGFXMaterialFunctions::TransformPath(TEXT("Transform/MF_MGFX_"));
FString FMGFXMaterialFunctions::VisualPath(TEXT("Visual/MF_MGFX_"));

TSoftObjectPtr<UMaterialFunctionInterface> FMGFXMaterialFunctions::GetFunction(const FString RelativePath)
{
	const FString ObjectName = FString(FPathViews::GetCleanFilename(RelativePath));
	const FString Path = MaterialFunctionsPath + RelativePath + "." + ObjectName;
	return TSoftObjectPtr<UMaterialFunctionInterface>(Path);
}

TSoftObjectPtr<UMaterialFunctionInterface> FMGFXMaterialFunctions::GetMerge(const FString Name)
{
	return GetFunction(MergePath + Name);
}

TSoftObjectPtr<UMaterialFunctionInterface> FMGFXMaterialFunctions::GetShape(const FString Name)
{
	return GetFunction(ShapePath + Name);
}

TSoftObjectPtr<UMaterialFunctionInterface> FMGFXMaterialFunctions::GetTransform(const FString Name)
{
	return GetFunction(TransformPath + Name);
}

TSoftObjectPtr<UMaterialFunctionInterface> FMGFXMaterialFunctions::GetVisual(const FString Name)
{
	return GetFunction(VisualPath + Name);
}
