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
 * Displays handles for translating, rotating, and scaling a 2D transform.
 * Designed to be placed in an untransformed SCanvas above a design area of any kind.
 * A parent transform can be provided in order to accurately represent a chain of transforms.
 *
 * Assign Location, Rotation, and Scale to supply the underlying local transform properties,
 * and assign OnSetLocation, OnSetRotation, OnSetScale to update them when the user drags this handle.
 *
 * The handle can be used externally by calling StartDragging and it does not need
 * to be visible when doing so, since transform calculations are done without the need
 * of this widget's actual geometry.
 */
class MGFXEDITOR_API SMGFXShapeTransformHandle : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FSetLocationDelegate, FVector2D /*NewLocation*/);
	DECLARE_DELEGATE_OneParam(FSetRotationDelegate, float /*NewRotation*/);
	DECLARE_DELEGATE_OneParam(FSetScaleDelegate, FVector2D /*NewScale*/);
	DECLARE_DELEGATE(FDragStartedDelegate);
	DECLARE_DELEGATE_OneParam(FDragFinishedDelegate, bool /*bWasModified*/);

public:
	SLATE_BEGIN_ARGS(SMGFXShapeTransformHandle)
			: _ParentTransform(FTransform2D()),
			  _HandleLength(80.f),
			  _HandleWidth(14.f),
			  _HandleLineWidth(2.f),
			  _ArrowSize(14.f)
		{
		}

		SLATE_ATTRIBUTE(EMGFXShapeTransformMode, Mode)

		/** The parent transform of the handle. */
		SLATE_ATTRIBUTE(FTransform2D, ParentTransform)

		/** The current location of the transform */
		SLATE_ATTRIBUTE(FVector2D, Location)

		/** The current rotation of the transform */
		SLATE_ATTRIBUTE(float, Rotation)

		/** The current scale of the transform */
		SLATE_ATTRIBUTE(FVector2D, Scale)

		/** Called to set the new location of the underlying transform. */
		SLATE_EVENT(FSetLocationDelegate, OnSetLocation)

		/** Called to set the new rotation of the underlying transform. */
		SLATE_EVENT(FSetRotationDelegate, OnSetRotation)

		/** Called to set the new scale of the underlying transform. */
		SLATE_EVENT(FSetScaleDelegate, OnSetScale)

		SLATE_EVENT(FDragStartedDelegate, OnDragStarted)
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
	FReply StartDragging(EMGFXShapeTransformHandle HandleType, const FPointerEvent& MouseEvent);

	/** Finish dragging the handle. */
	FReply FinishDragging();

protected:
	TSharedPtr<SWidget> TranslateXHandle;
	TSharedPtr<SWidget> TranslateYHandle;
	TSharedPtr<SWidget> RotateHandle;

	/**
	 * The parent transform of the target transform, including any view space transformations.
	 * This should not be applied to the SMGFXShapeTransformHandle widget, since only the rotation will
	 * need to be represented visually in the displayed handles.
	 */
	TAttribute<FTransform2D> ParentTransform;


	/** The current location of the transform. */
	TAttribute<FVector2D> Location;

	/** The current rotation of the transform. */
	TAttribute<float> Rotation;

	/** The current scale of the transform. */
	TAttribute<FVector2D> Scale;


	/** Called in order to update the location of the target transform when dragging this handle. */
	FSetLocationDelegate OnSetLocationEvent;

	/** Called in order to update the rotation of the target transform when dragging this handle. */
	FSetRotationDelegate OnSetRotationEvent;

	/** Called in order to update the scale of the target transform when dragging this handle. */
	FSetScaleDelegate OnSetScaleEvent;


	/** The location of the transform at the start of the current drag operation. */
	FVector2D OriginalLocation;

	/** The rotation of the transform at the start of the current drag operation. */
	float OriginalRotation;

	/** The scale of the transform at the start of the current drag operation. */
	FVector2D OriginalScale;


	/** The current transform mode, e.g. translate, rotate, or scale. */
	TAttribute<EMGFXShapeTransformMode> Mode;

	/** The currently active handle type being hovered or dragged. */
	TOptional<EMGFXShapeTransformHandle> ActiveHandle;

	/**
	 * If using modifier keys to affect the active handle type, this will be the original type without modifiers.
	 * Only set during drag operations.
	 */
	TOptional<EMGFXShapeTransformHandle> OriginalActiveHandle;

	bool bIsDragging = false;

	/** Were any changes made during the last drag operation? */
	bool bWasModified = false;

	/** The screen position of the mouse when the drag operation started. */
	FVector2D ScreenDragStartPosition;

	/**
	 * The local position of the mouse when the drag operation started.
	 * Optional because this will get filled after the first mouse move once dragging. 
	 */
	TOptional<FVector2D> LocalDragStartPosition;

	/** The local position of the mouse during the last mouse move. */
	FVector2D LocalDragPosition;

	/** The currently active transform transaction. */
	FScopedTransaction* Transaction;

	/** Called when a drag operation starts. */
	FDragStartedDelegate OnDragStartedEvent;

	/** Called when a drag operation has finished. */
	FDragFinishedDelegate OnDragFinishedEvent;

	/** The length of handles. */
	float HandleLength;

	/** Width of handles for hit testing purposes. */
	float HandleWidth;

	/** The visual width of handle lines. */
	float HandleLineWidth;

	/** The visual size of arrow heads. */
	float ArrowSize;

	/**
	 * Return the render transform to use for the handles widgets that are hit tested.
	 * This will only include parent space rotation.
	 */
	TOptional<FSlateRenderTransform> GetHandlesRenderTransform() const;

	/** Return the parent inverse transform including only it's rotation, used to visually rotate the handles. */
	FTransform2D GetParentInverseRotationTransform() const;

	/** Return the current color to use for a transform handle. */
	FSlateColor GetHandleColor(EMGFXShapeTransformHandle HandleType) const;

	/** Return whether a handle shold be visible. */
	EVisibility GetHandleVisibility(EMGFXShapeTransformHandle HandleType) const;

	FReply PerformMouseMoveTranslate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply PerformMouseMoveRotate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply PerformMouseMoveScale(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Snap a location to the grid. */
	FVector2D SnapLocationToGrid(FVector2D InLocation) const;

	/** Snap a rotation angle to the grid. */
	float SnapRotationToGrid(float InRotation) const;
};
