// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SMGFXShapeTransformHandle;
class FMGFXMaterialEditor;
class SArtboardPanel;
class SCanvas;
class SImage;
class UMGFXMaterial;
class UMGFXMaterialLayer;


/**
 * The main canvas for the MGFXMaterialEditor.
 */
class MGFXEDITOR_API SMGFXMaterialEditorCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorCanvas)
		{
		}

		SLATE_ARGUMENT(TWeakPtr<FMGFXMaterialEditor>, MGFXMaterialEditor)

	SLATE_END_ARGS()

	SMGFXMaterialEditorCanvas();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FVector2D GetArtboardSize() const;

	const FSlateBrush* GetArtboardBackground() const;

	bool ShouldShowArtboardBorder() const;

	TSharedPtr<SArtboardPanel> GetArtboardPanel() const { return ArtboardPanel; }

	/** Update the artboard size to match the material. */
	void UpdateArtboardSize();

	/** Called when the material asset has been set or changed. */
	void OnMaterialChanged(UMaterial* NewMaterial);

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditor;

	/** Return the original MGFXMaterial being edited. This should never be null. */
	UMGFXMaterial* GetMGFXMaterial() const;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	int32 PaintSelectionOutline(const FGeometry& AllottedGeometry, FSlateRect MyCullingRect,
	                            FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

protected:
	TSharedPtr<SArtboardPanel> ArtboardPanel;

	/** Canvas that contains transform and other interactive layer editing widgets. */
	TSharedPtr<SCanvas> EditingWidgetCanvas;

	/** The preview image displaying the material on the canvas. */
	TSharedPtr<SImage> PreviewImage;

	/** The handle for transforming the currently selected layer. */
	TSharedPtr<SMGFXShapeTransformHandle> TransformHandle;

	/** The slate brush displaying the preview image. */
	FSlateBrush PreviewImageBrush;

	/** Animates the width of selection outlines. */
	FCurveSequence SelectionOutlineAnim;

	bool bAlwaysShowArtboardBorder;

	void OnLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers);

	void CreateEditingWidgets();

	FVector2D GetEditingWidgetPosition(TSharedRef<SWidget> EditingWidget) const;

	FVector2D GetEditingWidgetSize(TSharedRef<SWidget> EditingWidget) const;

	/** Called to retrieve the selected layers transform when a transform handle drag begins. */
	FTransform2D OnGetLayerTransform() const;

	/** Called by transform handles to move the currently selected layer. */
	void OnLayerMoveTransform(FTransform2D NewTransform);

	/** Called when a transform handle is finished moving a layer. */
	void OnLayerMoveFinished();

	/** Return the currently selected layer (if there is only one). */
	UMGFXMaterialLayer* GetSelectedLayer() const;
};
