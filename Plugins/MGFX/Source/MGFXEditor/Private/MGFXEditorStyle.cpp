// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXEditorStyle.h"

#include "Styling/AppStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"


const FName FMGFXEditorStyle::StaticStyleSetName(TEXT("MGFXEditorStyle"));

FMGFXEditorStyle::FMGFXEditorStyle()
	: FSlateStyleSet(StaticStyleSetName)
{
	const FVector2D Icon20x20(20.f, 20.f);

	SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

#if WITH_EDITOR
	// MGFXMaterial Editor
	{
		Set(TEXT("MGFXMaterialEditor.RegenerateMaterial"), new IMAGE_BRUSH_SVG("Starship/Common/Apply", Icon20x20));

		Set(TEXT("ArtboardBackground"), new FSlateColorBrush(FLinearColor(0.005f, 0.005f, 0.005f)));

		// TODO: use an invisible brush? paint the handle manually?
		auto* TranslateBrush = new FSlateColorBrush(FLinearColor::White);
		TranslateBrush->SetImageSize(FVector2D(100.f, 6.f));
		Set(TEXT("ShapeTransformHandle.Translate"), TranslateBrush);

		const FSlateColor SelectionColor = FAppStyle::GetSlateColor("SelectionColor");
		const FSlateColor SelectorColor = FAppStyle::GetSlateColor("SelectorColor");

		// create a rounded-corner TableView.Row style for material editor layers
		auto MakeRoundedCorners = [](FSlateBrush& SlateBrush)
		{
			SlateBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			SlateBrush.OutlineSettings.CornerRadii = FVector4(2.f, 2.f, 2.f, 2.f);
			SlateBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		};

		FTableRowStyle LayerTableViewRowStyle = FTableRowStyle(FAppStyle::GetWidgetStyle<FTableRowStyle>("TableView.Row"));

		LayerTableViewRowStyle
			.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Panel))
			.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Hover))
			.SetOddRowBackgroundBrush(FSlateColorBrush(FStyleColors::Panel))
			.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Hover));

		MakeRoundedCorners(LayerTableViewRowStyle.ParentRowBackgroundBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.ParentRowBackgroundHoveredBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.EvenRowBackgroundBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.EvenRowBackgroundHoveredBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.OddRowBackgroundBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.OddRowBackgroundHoveredBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.ActiveBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.ActiveHoveredBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.InactiveBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.InactiveHoveredBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.ActiveHighlightedBrush);
		MakeRoundedCorners(LayerTableViewRowStyle.InactiveHighlightedBrush);

		MakeRoundedCorners(LayerTableViewRowStyle.SelectorFocusedBrush);

		Set(TEXT("MGFXMaterialEditor.Layers.TableViewRow"), LayerTableViewRowStyle);
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
