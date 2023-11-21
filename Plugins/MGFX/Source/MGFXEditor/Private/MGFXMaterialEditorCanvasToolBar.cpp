// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditorCanvasToolBar.h"

#include "EditorViewportCommands.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMGFXMaterialEditorCanvasToolBar::Construct(const FArguments& InArgs)
{
	FToolBarBuilder ToolbarBuilder(InArgs._CommandList, FMultiBoxCustomization::None, InArgs._Extenders);

	ToolbarBuilder.SetStyle(&FAppStyle::Get(), TEXT("EditorViewportToolBar"));
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	ToolbarBuilder.SetIsFocusable(false);

	ToolbarBuilder.BeginSection("Transform");
	{
		ToolbarBuilder.BeginBlockGroup();

		// Select Mode
		static FName SelectModeName = FName(TEXT("SelectMode"));
		ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().SelectMode, NAME_None,
		                                TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), SelectModeName);

		// Translate Mode
		static FName TranslateModeName = FName(TEXT("TranslateMode"));
		ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().TranslateMode, NAME_None,
		                                TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), TranslateModeName);

		// Rotate Mode
		static FName RotateModeName = FName(TEXT("RotateMode"));
		ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().RotateMode, NAME_None,
		                                TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), RotateModeName);

		// Scale Mode
		static FName ScaleModeName = FName(TEXT("ScaleMode"));
		ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().ScaleMode, NAME_None,
		                                TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), ScaleModeName);


		ToolbarBuilder.EndBlockGroup();
		ToolbarBuilder.AddSeparator();
	}
	ToolbarBuilder.EndSection();

	ChildSlot
	[
		ToolbarBuilder.MakeWidget()
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
