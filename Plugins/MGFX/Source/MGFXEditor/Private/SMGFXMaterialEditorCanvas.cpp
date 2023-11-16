// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorCanvas.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "SArtboardPanel.h"
#include "SlateOptMacros.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMGFXMaterialEditorCanvas::Construct(const FArguments& InArgs)
{
	MGFXMaterialEditorPtr = InArgs._MGFXMaterialEditor;
	check(MGFXMaterialEditorPtr.IsValid());

	PreviewImageBrush.SetResourceObject(MGFXMaterialEditorPtr.Pin()->GetGeneratedMaterial());

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(4.f)
		[
			SAssignNew(ArtboardPanel, SArtboardPanel)
			.ArtboardSize(this, &SMGFXMaterialEditorCanvas::GetArtboardSize)
			.BackgroundBrush(FSlateColorBrush(FLinearColor(0.002f, 0.002f, 0.002f)))
			.bShowArtboardBorder(this, &SMGFXMaterialEditorCanvas::ShouldShowArtboardBorder)
			.Clipping(EWidgetClipping::ClipToBounds)

			// add preview material image, full size of the artboard
			+ SArtboardPanel::Slot()
			  .Position(FVector2D::ZeroVector)
			  .Size(GetArtboardSize())
			[
				SAssignNew(PreviewImage, SImage)
				.Image(&PreviewImageBrush)
			]
		]
	];
}

FVector2D SMGFXMaterialEditorCanvas::GetArtboardSize() const
{
	return MGFXMaterialEditorPtr.IsValid() ? MGFXMaterialEditorPtr.Pin()->GetCanvasSize() : FVector2D(512, 512);
}

bool SMGFXMaterialEditorCanvas::ShouldShowArtboardBorder() const
{
	// when the preview image doesn't match the current artboard settings, force display the border
	const SArtboardPanel::FSlot* Slot = ArtboardPanel->GetWidgetSlot(PreviewImage);
	return Slot->GetSize() != GetArtboardSize();
}

void SMGFXMaterialEditorCanvas::UpdateArtboardSize()
{
	if (const UMGFXMaterial* Material = GetMGFXMaterial())
	{
		SArtboardPanel::FSlot* Slot = ArtboardPanel->GetWidgetSlot(PreviewImage);
		check(Slot);
		Slot->SetSize(Material->BaseCanvasSize);
	}
}

UMGFXMaterial* SMGFXMaterialEditorCanvas::GetMGFXMaterial() const
{
	return MGFXMaterialEditorPtr.Pin()->GetOriginalMGFXMaterial();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
