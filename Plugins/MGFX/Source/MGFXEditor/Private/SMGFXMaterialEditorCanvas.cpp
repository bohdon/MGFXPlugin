// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorCanvas.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "SArtboardPanel.h"
#include "SlateOptMacros.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMGFXMaterialEditorCanvas::Construct(const FArguments& InArgs)
{
	check(InArgs._MGFXMaterialEditor.IsValid());

	MGFXMaterialEditor = InArgs._MGFXMaterialEditor;

	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddRaw(this, &SMGFXMaterialEditorCanvas::OnLayerSelectionChanged);
	MGFXMaterialEditor.Pin()->OnMaterialChangedEvent.AddRaw(this, &SMGFXMaterialEditorCanvas::OnMaterialChanged);

	PreviewImageBrush.SetResourceObject(MGFXMaterialEditor.Pin()->GetGeneratedMaterial());

	SelectionOutlineAnim = FCurveSequence(0.0f, 0.15f, ECurveEaseFunction::QuadOut);

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
			.ZoomAmountMax(100.f)
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
	return MGFXMaterialEditor.IsValid() ? MGFXMaterialEditor.Pin()->GetCanvasSize() : FVector2D(512, 512);
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

void SMGFXMaterialEditorCanvas::OnMaterialChanged(UMaterial* NewMaterial)
{
	PreviewImageBrush.SetResourceObject(NewMaterial);
}

UMGFXMaterial* SMGFXMaterialEditorCanvas::GetMGFXMaterial() const
{
	return MGFXMaterialEditor.Pin()->GetMGFXMaterial();
}

int32 SMGFXMaterialEditorCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                         FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	++LayerId;
	LayerId = PaintSelectionOutline(ArtboardPanel->GetPaintSpaceGeometry(), MyCullingRect, OutDrawElements, LayerId);

	return LayerId;
}

int32 SMGFXMaterialEditorCanvas::PaintSelectionOutline(const FGeometry& AllottedGeometry, FSlateRect MyCullingRect,
                                                       FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();

	const FLinearColor OutlineColor = FStyleColors::Select.GetSpecifiedColor();

	// flash a broad width selection outline to draw attention
	const float OutlineWidth = FMath::Lerp(3.f, 1.f, SelectionOutlineAnim.GetLerp());

	for (const UMGFXMaterialLayer* SelectedLayer : SelectedLayers)
	{
		if (!SelectedLayer->HasBounds())
		{
			// no bounds to represent
			continue;
		}

		// layer transform and shape bounds
		const FTransform2D LayerTransform = SelectedLayer->GetTransform();
		const FBox2D LayerBounds = SelectedLayer->GetBounds();

		// apply artboard pan/zoom transform
		const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
		FGeometry LayerGeometry = AllottedGeometry
		                          .MakeChild(ArtboardTransform, FVector2f::ZeroVector)
		                          .MakeChild(LayerTransform, FVector2f::ZeroVector)
		                          .MakeChild(LayerBounds.GetSize(), FSlateLayoutTransform(LayerBounds.Min));

		// convert to paint geometry, and inflate by outline size
		const FVector2D LayerScaleVector = FVector2D(LayerGeometry.GetAccumulatedRenderTransform().GetMatrix().GetScale().GetVector());
		const FVector2D OutlinePixelSize = FVector2D(OutlineWidth, OutlineWidth) / LayerScaleVector;
		FPaintGeometry SelectionGeometry = LayerGeometry.ToInflatedPaintGeometry(OutlinePixelSize);

		FSlateClippingZone SelectionZone(SelectionGeometry);
		const TArray<FVector2D> Points = {
			FVector2D(SelectionZone.TopLeft),
			FVector2D(SelectionZone.TopRight),
			FVector2D(SelectionZone.BottomRight),
			FVector2D(SelectionZone.BottomLeft),
			FVector2D(SelectionZone.TopLeft),
		};

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			FPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			OutlineColor,
			true,
			OutlineWidth);
	}

	return LayerId;
}

void SMGFXMaterialEditorCanvas::OnLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers)
{
	SelectionOutlineAnim.Play(SharedThis(this));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
