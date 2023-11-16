// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SPanel.h"


/**
 * A panel for displaying design elements, with controls for panning and zooming.
 * Similar to SNodePanel, but designed for generic elements instead of just nodes.
 */
class MGFXEDITOR_API SArtboardPanel : public SPanel
{
public:
	/**
	 * An SArtboardPanel slot. Contains a position and size.
	 */
	class FSlot : public TWidgetSlotWithAttributeSupport<FSlot>
	{
	public:
		SLATE_SLOT_BEGIN_ARGS(FSlot, TSlotBase<FSlot>)
			SLATE_ATTRIBUTE(FVector2D, Position)
			SLATE_ATTRIBUTE(FVector2D, Size)

		SLATE_END_ARGS()

		FSlot()
			: TWidgetSlotWithAttributeSupport<FSlot>(),
			  Position(*this, FVector2D::ZeroVector),
			  Size(*this, FVector2D(1.0f, 1.0f))
		{
		}

		void SetPosition(TAttribute<FVector2D> InPosition)
		{
			Position.Assign(*this, MoveTemp(InPosition));
		}

		FVector2D GetPosition() const
		{
			return Position.Get();
		}

		void SetSize(TAttribute<FVector2D> InSize)
		{
			Size.Assign(*this, MoveTemp(InSize));
		}

		FVector2D GetSize() const
		{
			return Size.Get();
		}

		void Construct(const FChildren& SlotOwner, FSlotArguments&& InArg);
		static void RegisterAttributes(FSlateWidgetSlotAttributeInitializer& AttributeInitializer);

	private:
		TSlateSlotAttribute<FVector2D> Position;

		TSlateSlotAttribute<FVector2D> Size;
	};

	SLATE_BEGIN_ARGS(SArtboardPanel)
		{
		}

		SLATE_ATTRIBUTE(FVector2D, ArtboardSize)
		SLATE_ATTRIBUTE(FSlateBrush, BackgroundBrush)
		SLATE_ATTRIBUTE(bool, bShowArtboardBorder)
		SLATE_SLOT_ARGUMENT(FSlot, Slots)

	SLATE_END_ARGS()

	SArtboardPanel();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> ConstructOverlay();

	/** Remove all children from the panel */
	void ClearChildren();

	static FSlot::FSlotArguments Slot();

	/** Return the slot of a child widget. */
	SArtboardPanel::FSlot* GetWidgetSlot(const TSharedPtr<SWidget>& Widget);

	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FChildren* GetChildren() override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual int32 PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                              FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;


	FVector2D GetViewOffset() const { return ViewOffset; }

	void SetViewOffset(FVector2D NewViewOffset, const FGeometry& AllottedGeometry);

	float GetZoomAmount() const { return ZoomAmount; }

	void SetZoomAmount(float NewZoomAmount);

	void ApplyZoomDelta(float ZoomDelta, const FVector2D& LocalFocalPosition, const FGeometry& AllottedGeometry);

	FVector2D GraphCoordToPanelCoord(const FVector2D& GraphPosition) const;

	FVector2D PanelCoordToGraphCoord(const FVector2D& PanelPosition) const;

protected:
	/** The current viewing offset of the panel. */
	FVector2D ViewOffset;

	/** The minimum margin to preserve when panning the view offset. */
	FVector2D ViewOffsetMargin;

	/** The current viewing zoom level, e.g. 0.5 results in half-sized elements. */
	float ZoomAmount;

	/** The minimum allowed zoom amount. */
	float ZoomAmountMin;

	/** The maximum allowed zoom amount. */
	float ZoomAmountMax;

	/** Is a panning operation in progress? */
	bool bIsPanning;

	/** Is a zoom operation in progress? */
	bool bIsZooming;

	/** Absolute position of the mouse when the panning operation started. */
	FVector2D PanStartPosition;

	/** The view offset at the start of the current panning operation. */
	FVector2D PanViewOffsetStart;

	/** Sensitivity of drag zoom operation. */
	float ZoomSensitivity;

	/** Sensitivity of mouse wheel zoom operation. */
	float ZoomWheelSensitivity;

	/** Local position in the panel where the zoom operation began and zoom in/out towards. */
	FVector2D ZoomFocalPosition;

	/** The size of the artboard. */
	TAttribute<FVector2D> ArtboardSize;

	TPanelChildren<FSlot> Children;

	TAttribute<FSlateBrush> BackgroundBrush;

	TAttribute<bool> bShowArtboardBorder;

	virtual void OnViewOffsetChanged();

	virtual void OnZoomChanged();

	virtual FText GetDebugText() const;
};
