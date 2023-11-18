// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UMGFXMaterialLayer;
class FMGFXMaterialEditor;
class SArtboardPanel;
class SImage;
class UMGFXMaterial;


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

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FVector2D GetArtboardSize() const;

	bool ShouldShowArtboardBorder() const;

	TSharedPtr<SArtboardPanel> GetArtboardPanel() const { return ArtboardPanel; }

	/** Update the artboard size to match the material. */
	void UpdateArtboardSize();

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditor;

	/** Return the original MGFXMaterial being edited. This should never be null. */
	UMGFXMaterial* GetMGFXMaterial() const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	int32 PaintSelectionOutline(const FGeometry& AllottedGeometry, FSlateRect MyCullingRect,
	                            FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

protected:
	TSharedPtr<SArtboardPanel> ArtboardPanel;

	/** The preview image displaying the material on the canvas. */
	TSharedPtr<SImage> PreviewImage;

	/** The slate brush displaying the preview image. */
	FSlateBrush PreviewImageBrush;

	/** Animates the width of selection outlines. */
	FCurveSequence SelectionOutlineAnim;

	void OnLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers);
};
