// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorCanvas.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "SArtboardPanel.h"
#include "SlateOptMacros.h"
#include "SMGFXShapeTransformHandle.h"
#include "Widgets/SCanvas.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXMaterialEditorCanvas::SMGFXMaterialEditorCanvas()
	: bAlwaysShowArtboardBorder(true)
{
}

void SMGFXMaterialEditorCanvas::Construct(const FArguments& InArgs)
{
	check(InArgs._MGFXMaterialEditor.IsValid());

	MGFXMaterialEditor = InArgs._MGFXMaterialEditor;

	// TODO: expose as view option
	bAlwaysShowArtboardBorder = true;

	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddSP(this, &SMGFXMaterialEditorCanvas::OnLayerSelectionChanged);
	MGFXMaterialEditor.Pin()->OnMaterialChangedEvent.AddSP(this, &SMGFXMaterialEditorCanvas::OnMaterialChanged);

	PreviewImageBrush.SetResourceObject(MGFXMaterialEditor.Pin()->GetGeneratedMaterial());

	SelectionOutlineAnim = FCurveSequence(0.0f, 0.15f, ECurveEaseFunction::QuadOut);

	UMGFXMaterial* MGFXMaterial = MGFXMaterialEditor.Pin()->GetMGFXMaterial();

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(ArtboardPanel, SArtboardPanel)
			.ArtboardSize(this, &SMGFXMaterialEditorCanvas::GetArtboardSize)
			.BackgroundBrush(this, &SMGFXMaterialEditorCanvas::GetArtboardBackground)
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

		+ SOverlay::Slot()
		[
			SAssignNew(EditingWidgetCanvas, SCanvas)
		]
	];
}

FVector2D SMGFXMaterialEditorCanvas::GetArtboardSize() const
{
	return MGFXMaterialEditor.Pin()->GetCanvasSize();
}

const FSlateBrush* SMGFXMaterialEditorCanvas::GetArtboardBackground() const
{
	const UMGFXMaterial* MGFXMaterial = MGFXMaterialEditor.Pin()->GetMGFXMaterial();
	return MGFXMaterial->bOverrideDesignerBackground ? &MGFXMaterial->DesignerBackground : nullptr;
}

bool SMGFXMaterialEditorCanvas::ShouldShowArtboardBorder() const
{
	if (bAlwaysShowArtboardBorder)
	{
		return true;
	}

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

FReply SMGFXMaterialEditorCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// middle mouse to drag move the selected layer
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && !MouseEvent.IsAltDown())
	{
		if (GetSelectedLayer() && TransformHandle.IsValid())
		{
			// TODO: use shift to limit to an axis
			return TransformHandle->StartDragging(EMGFXShapeTransformHandle::TranslateXY, MouseEvent);
		}

		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
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

	// clear and recreate widgets to interactively transform or modify the layer
	CreateEditingWidgets();
}

void SMGFXMaterialEditorCanvas::CreateEditingWidgets()
{
	// clear existing widgets
	TransformHandle.Reset();
	EditingWidgetCanvas->ClearChildren();

	if (!GetSelectedLayer())
	{
		return;
	}

	// create transform handle
	SAssignNew(TransformHandle, SMGFXShapeTransformHandle)
		.OnGetTransform(this, &SMGFXMaterialEditorCanvas::OnGetLayerTransform)
		.OnMoveTransform(this, &SMGFXMaterialEditorCanvas::OnLayerMoveTransform)
		.OnDragFinished(this, &SMGFXMaterialEditorCanvas::OnLayerMoveFinished);

	const TSharedRef<SMGFXShapeTransformHandle> TransformHandleRef = TransformHandle.ToSharedRef();
	EditingWidgetCanvas
		->AddSlot()
		.Position(this, &SMGFXMaterialEditorCanvas::GetEditingWidgetPosition, StaticCastSharedRef<SWidget>(TransformHandleRef))
		.Size(this, &SMGFXMaterialEditorCanvas::GetEditingWidgetSize, StaticCastSharedRef<SWidget>(TransformHandleRef))
		[
			TransformHandleRef
		];
}

FVector2D SMGFXMaterialEditorCanvas::GetEditingWidgetPosition(TSharedRef<SWidget> EditingWidget) const
{
	const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer();
	if (!SelectedLayer)
	{
		return FVector2D::ZeroVector;
	}

	// position will be relative to this canvas's geometry, so just perform the canvas -> layer position transformation
	const FTransform2D LayerTransform = SelectedLayer->GetTransform();
	const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
	const FVector2D Position = LayerTransform.Concatenate(ArtboardTransform).TransformPoint(FVector2D::ZeroVector);

	return Position;
}

FVector2D SMGFXMaterialEditorCanvas::GetEditingWidgetSize(TSharedRef<SWidget> EditingWidget) const
{
	return EditingWidget->GetDesiredSize();
}

FTransform2D SMGFXMaterialEditorCanvas::OnGetLayerTransform() const
{
	if (const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		// transforms are edited in view space, e.g. with artboard transform applied
		const FTransform2D LayerTransform = SelectedLayer->GetTransform();
		const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
		return LayerTransform.Concatenate(ArtboardTransform);
	}
	return FTransform2D();
}

void SMGFXMaterialEditorCanvas::OnLayerMoveTransform(FTransform2D NewTransform)
{
	if (UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		// transforms are given in view space, remove artboard and parent transform to convert back to local
		const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
		const FTransform2D FullParentTransform = SelectedLayer->GetParentTransform().Concatenate(ArtboardTransform);

		const FTransform2D NewLayerTransform = NewTransform.Concatenate(FullParentTransform.Inverse());
		const FVector2f NewLocation = FVector2f(NewLayerTransform.GetTranslation());

		if (NewLocation != SelectedLayer->Transform.Location)
		{
			SelectedLayer->Modify();
			SelectedLayer->Transform.Location = NewLocation;
		}
	}
}

void SMGFXMaterialEditorCanvas::OnLayerMoveFinished()
{
	if (UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		// TODO: broadcast event instead?
		MGFXMaterialEditor.Pin()->RegenerateMaterial();
	}
}

UMGFXMaterialLayer* SMGFXMaterialEditorCanvas::GetSelectedLayer() const
{
	TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	if (SelectedLayers.Num() == 1)
	{
		return SelectedLayers[0];
	}
	return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
