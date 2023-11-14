// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXEditorStyle.h"

#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"


const FName FMGFXEditorStyle::StaticStyleSetName(TEXT("MGFXEditorStyle"));

FMGFXEditorStyle::FMGFXEditorStyle()
	: FSlateStyleSet(StaticStyleSetName)
{
	const FVector2D Icon20x20(20.f, 20.f);

	SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

#if WITH_EDITOR
	{
		Set(TEXT("MGFXMaterialEditor.RegenerateMaterial"), new IMAGE_BRUSH_SVG("Starship/Common/Apply", Icon20x20));
	}
#endif

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FMGFXEditorStyle::~FMGFXEditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

FMGFXEditorStyle& FMGFXEditorStyle::Get()
{
	static FMGFXEditorStyle Inst;
	return Inst;
}
