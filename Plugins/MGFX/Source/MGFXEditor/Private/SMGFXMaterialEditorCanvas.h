// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SMGFXShapeTransformHandle.h"
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

	TSharedRef<SWidget> CreateOverlayUI();

	FVector2D GetArtboardSize() const;

	const FSlateBrush* GetArtboardBackground() const;

	bool ShouldShowArtboardBorder() const;

	TSharedPtr<SArtboardPanel> GetArtboardPanel() const { return ArtboardPanel; }

	/** Update the artboard size to match the material. */
	void UpdateArtboardSize();

	/** Called when the editor preview material has been set or changed. */
	void OnPreviewMaterialChanged(UMaterialInterface* NewMaterial);

	/** Weak pointer to the owning material editor. */
	TWeakPtr<FMGFXMaterialEditor> MGFXMaterialEditor;

	/** Return the original MGFXMaterial being edited. This should never be null. */
	UMGFXMaterial* GetMGFXMaterial() const;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	int32 PaintSelectionOutline(const FGeometry& AllottedGeometry, FSlateRect MyCullingRect,
	                            FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

protected:
	/** Commandlist used in the canvas. */
	TSharedPtr<FUICommandList> CommandList;

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

	/** The current transform editing mode. */
	EMGFXShapeTransformMode TransformMode;

	bool bAlwaysShowArtboardBorder;

	void BindCommands();

	void OnViewOffsetChanged(FVector2D NewViewOffset);

	void OnZoomChanged(float NewZoomAmount);

	void OnLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers);

	void CreateEditingWidgets();

	FVector2D GetEditingWidgetPosition(TSharedRef<SWidget> EditingWidget) const;

	FVector2D GetEditingWidgetSize(TSharedRef<SWidget> EditingWidget) const;

	/**
	 * Called to retrieve the selected layer's parent transform for use with the transform handle.
	 * This will include artboard view offset and scale as well.
	 */
	FTransform2D GetTransformHandleParentTransform() const;

	FVector2D GetTransformHandleLocation() const;
	float GetTransformHandleRotation() const;
	FVector2D GetTransformHandleScale() const;

	/** Called by transform handles to move the currently selected layer. */
	void OnSetLayerLocation(FVector2D NewLocation, bool bIsFinished);

	/** Called by transform handles to rotate the currently selected layer. */
	void OnSetLayerRotation(float NewRotation, bool bIsFinished);

	/** Called by transform handles to scale the currently selected layer. */
	void OnSetLayerScale(FVector2D NewScale, bool bIsFinished);

	void SendLayerTransformPropertyChangeEvent(UMGFXMaterialLayer* Layer, EPropertyChangeType::Type ChangeType, FProperty* Property);

	/** Return the currently selected layer (if there is only one). */
	UMGFXMaterialLayer* GetSelectedLayer() const;

	EMGFXShapeTransformMode GetTransformMode() const { return TransformMode; }

	void SetTransformMode(EMGFXShapeTransformMode NewMode);

	bool CanSetTransformMode(EMGFXShapeTransformMode NewMode);

	bool IsTransformModeActive(EMGFXShapeTransformMode InMode);

	FText GetZoomPresetsMenULabel() const;

	TSharedRef<SWidget> CreateZoomPresetsMenu();

	/** Set a new view scale for the artboard panel. */
	void SetZoomAmount(float NewZoomAmount);
};
