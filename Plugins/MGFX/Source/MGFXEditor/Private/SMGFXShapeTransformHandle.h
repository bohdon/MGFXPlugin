// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"


enum class EMGFXShapeTransformMode : uint8
{
	Select,
	Translate,
	Rotate,
	Scale
};

enum class EMGFXShapeTransformHandle : uint8
{
	TranslateX,
	TranslateY,
	TranslateXY,
	Rotate,
	ScaleX,
	ScaleY,
	ScaleXY,
};

/**
 * Displays a handle for translating, rotating, and scaling a 2D object or shape.
 */
class MGFXEDITOR_API SMGFXShapeTransformHandle : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_RetVal(FTransform2D, FGetTransformDelegate);
	DECLARE_DELEGATE_OneParam(FMoveTransformDelegate, FTransform2D /*NewTransform*/);
	DECLARE_DELEGATE(FDragFinishedDelegate);

public:
	SLATE_BEGIN_ARGS(SMGFXShapeTransformHandle)
			: _HandleLength(80.f),
			  _HandleWidth(14.f),
			  _HandleLineWidth(2.f),
			  _ArrowSize(14.f)
		{
		}

		SLATE_ATTRIBUTE(EMGFXShapeTransformMode, Mode)
		SLATE_EVENT(FGetTransformDelegate, OnGetTransform)
		SLATE_EVENT(FMoveTransformDelegate, OnMoveTransform)
		SLATE_EVENT(FDragFinishedDelegate, OnDragFinished)
		SLATE_ARGUMENT(float, HandleLength)
		SLATE_ARGUMENT(float, HandleWidth)
		SLATE_ARGUMENT(float, HandleLineWidth)
		SLATE_ARGUMENT(float, ArrowSize)

	SLATE_END_ARGS()

	SMGFXShapeTransformHandle();

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** Paint an arrow with a direction and color. Uses the HandleLength and ArrowSize properties. */
	int32 PaintTranslateHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                           int32 LayerId, const FVector2D& Direction, const FLinearColor& Color) const;

	/** Paint a rotation handle. */
	int32 PaintRotateHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                        int32 LayerId, const FLinearColor& Color) const;

	/** Return true if two handle types are equal, or if HandleType is a sub category of OtherHandleType, e.g. TranslateX is a subtype of TranslateXY. */
	bool IsMatchingOrRelevantHandle(EMGFXShapeTransformHandle HandleType, EMGFXShapeTransformHandle OtherHandleType) const;

	TOptional<EMGFXShapeTransformHandle> GetHoveredHandle() const;

	/** Start a drag operation for the active transform mode. */
	FReply StartDragging(const FPointerEvent& MouseEvent);

	/** Start dragging this transform handle. */
	FReply StartDragging(EMGFXShapeTransformHandle HandleType, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Finish dragging the handle. */
	FReply FinishDragging();

	/** Called to retrieve the starting transform when a drag begins. */
	FGetTransformDelegate OnGetTransform;

	/** Called in order to update the transform of the target object when dragging this handle. */
	FMoveTransformDelegate OnMoveTransform;

	/** Called when a drag operation has finished. */
	FDragFinishedDelegate OnDragFinished;

protected:
	TSharedPtr<SWidget> TranslateXHandle;
	TSharedPtr<SWidget> TranslateYHandle;
	TSharedPtr<SWidget> RotateHandle;

	/**
	 * If using modifier keys to affect the active handle type, this will be the original type without modifiers.
	 * Only set during drag operations.
	 */
	TOptional<EMGFXShapeTransformHandle> OriginalActiveHandle;

	/** The currently active handle type being hovered or dragged. */
	TOptional<EMGFXShapeTransformHandle> ActiveHandle;

	/** The current transform mode, e.g. translate, rotate, or scale. */
	TAttribute<EMGFXShapeTransformMode> Mode;

	/** The length of handles. */
	float HandleLength;

	/** Width of handles for hit testing purposes. */
	float HandleWidth;

	/** The visual width of handle lines. */
	float HandleLineWidth;

	/** The visual size of arrow heads. */
	float ArrowSize;

	bool bIsDragging = false;

	/**
	 * The parent transform in which to perform transformations during drag operations.
	 * Used to allow input coordinates to be converted, rather than performing lossy transformations later.
	 */
	FTransform2D DragParentTransform;

	/** The local transform of the object when dragging started. */
	FTransform2D DragStartTransform;

	/** The screen position of the mouse when the drag operation started. */
	FVector2D ScreenDragStartPosition;

	/** The local position of the mouse when the drag operation started. */
	FVector2D LocalDragStartPosition;

	/** The local position of the mouse during the last mouse move. */
	FVector2D LocalDragPosition;

	/** The currently active transform transaction. */
	FScopedTransaction* Transaction;

	/** Return the current color to use for a transform handle. */
	FSlateColor GetHandleColor(EMGFXShapeTransformHandle HandleType) const;

	/** Return whether a handle shold be visible. */
	EVisibility GetHandleVisibility(EMGFXShapeTransformHandle HandleType) const;

	FReply PerformMouseMoveTranslate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply PerformMouseMoveRotate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply PerformMouseMoveScale(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
};
