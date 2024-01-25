// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXShapeTransformHandle.h"

#include "MGFXEditorModule.h"
#include "ScopedTransaction.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Images/SImage.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXShapeTransformHandle::SMGFXShapeTransformHandle()
	: OriginalRotation(0),
	  Transaction(nullptr),
	  HandleLength(80.f),
	  HandleWidth(14.f),
	  HandleLineWidth(2.f),
	  ArrowSize(14.f)
{
}

void SMGFXShapeTransformHandle::Construct(const FArguments& InArgs)
{
	if (!InArgs._OnSetLocation.IsBound() ||
		!InArgs._OnSetRotation.IsBound() ||
		!InArgs._OnSetLocation.IsBound())
	{
		UE_LOG(LogMGFXEditor, Error, TEXT("OnSetLocation/Rotation/Scale must all be set."))
	}

	Mode = InArgs._Mode;

	ParentTransform = InArgs._ParentTransform;
	Location = InArgs._Location;
	Rotation = InArgs._Rotation;
	Scale = InArgs._Scale;
	OnSetLocationEvent = InArgs._OnSetLocation;
	OnSetRotationEvent = InArgs._OnSetRotation;
	OnSetScaleEvent = InArgs._OnSetScale;

	OnDragStartedEvent = InArgs._OnDragStarted;
	OnDragFinishedEvent = InArgs._OnDragFinished;

	HandleLength = InArgs._HandleLength;
	HandleWidth = InArgs._HandleWidth;
	HandleLineWidth = InArgs._HandleLineWidth;
	ArrowSize = InArgs._ArrowSize;

	// handle SImages below are for hit testing only,
	// lines are drawn in OnPaint that are thinner and more appealing

	ChildSlot
	[
		SNew(SCanvas)
		.RenderTransform(this, &SMGFXShapeTransformHandle::GetHandlesRenderTransform, EMGFXShapeTransformHandle::TranslateXY)

		// translate Y handle
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleWidth, HandleLength + ArrowSize))
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SAssignNew(TranslateYHandle, SImage)
			.Visibility(this, &SMGFXShapeTransformHandle::GetHandleVisibility, EMGFXShapeTransformHandle::TranslateY)
			.ColorAndOpacity(FLinearColor::Transparent)
		]

		// translate X handle
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleLength + ArrowSize, HandleWidth))
		  .HAlign(HAlign_Left)
		  .VAlign(VAlign_Center)
		[
			SAssignNew(TranslateXHandle, SImage)
			.Visibility(this, &SMGFXShapeTransformHandle::GetHandleVisibility, EMGFXShapeTransformHandle::TranslateX)
			.ColorAndOpacity(FLinearColor::Transparent)
		]

		// TODO: actual circle hit test
		// rotate handle
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleLength, HandleLength) * 2.f + HandleWidth)
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		[
			SAssignNew(RotateHandle, SImage)
			.Visibility(this, &SMGFXShapeTransformHandle::GetHandleVisibility, EMGFXShapeTransformHandle::Rotate)
			.ColorAndOpacity(FLinearColor::Transparent)
		]

		// scale Y handle
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleWidth, HandleLength + ArrowSize))
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SAssignNew(ScaleYHandle, SImage)
			.Visibility(this, &SMGFXShapeTransformHandle::GetHandleVisibility, EMGFXShapeTransformHandle::ScaleY)
			.ColorAndOpacity(FLinearColor::Transparent)
			.RenderTransformPivot(FVector2D(0.5f, 1.f))
			.RenderTransform(this, &SMGFXShapeTransformHandle::GetHandlesRenderTransform, EMGFXShapeTransformHandle::ScaleY)
		]

		// scale X handle
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleLength + ArrowSize, HandleWidth))
		  .HAlign(HAlign_Left)
		  .VAlign(VAlign_Center)
		[
			SAssignNew(ScaleXHandle, SImage)
			.Visibility(this, &SMGFXShapeTransformHandle::GetHandleVisibility, EMGFXShapeTransformHandle::ScaleX)
			.ColorAndOpacity(FLinearColor::Transparent)
			.RenderTransformPivot(FVector2D(0.f, 0.5f))
			.RenderTransform(this, &SMGFXShapeTransformHandle::GetHandlesRenderTransform, EMGFXShapeTransformHandle::ScaleX)
		]
	];
}

FReply SMGFXShapeTransformHandle::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// determine which handle to drag
		const TOptional<EMGFXShapeTransformHandle> HandleType = GetHoveredHandle();

		// start dragging
		if (HandleType.IsSet())
		{
			return StartDragging(HandleType.GetValue(), MouseEvent);
		}
	}

	return FReply::Unhandled();
	// return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SMGFXShapeTransformHandle::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		return FinishDragging();
	}

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SMGFXShapeTransformHandle::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		check(OriginalActiveHandle.IsSet());

		if (!LocalDragStartPosition.IsSet())
		{
			// fill out start positions using previous mouse position.
			// done here because we have MyGeometry to convert to local space
			LocalDragStartPosition = MyGeometry.AbsoluteToLocal(ScreenDragStartPosition);
			LocalDragPosition = LocalDragStartPosition.GetValue();
		}

		LocalDragPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		// perform drag operation based on the handle type that initiated the drag
		// (not based on the current mode, to avoid switching mid-operation)
		switch (OriginalActiveHandle.GetValue())
		{
		case EMGFXShapeTransformHandle::TranslateXY:
		case EMGFXShapeTransformHandle::TranslateX:
		case EMGFXShapeTransformHandle::TranslateY:
			return PerformMouseMoveTranslate(MyGeometry, MouseEvent);
			break;
		case EMGFXShapeTransformHandle::Rotate:
			return PerformMouseMoveRotate(MyGeometry, MouseEvent);
			break;
		case EMGFXShapeTransformHandle::ScaleXY:
		case EMGFXShapeTransformHandle::ScaleX:
		case EMGFXShapeTransformHandle::ScaleY:
			return PerformMouseMoveScale(MyGeometry, MouseEvent);
			break;
		default:
			return FReply::Unhandled();
		}
	}
	else
	{
		// highlight the hovered handle
		ActiveHandle = GetHoveredHandle();
	}

	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
}

FReply SMGFXShapeTransformHandle::PerformMouseMoveTranslate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// get the raw delta, using screen space coords
	FVector2D DragDelta = MouseEvent.GetScreenSpacePosition() - ScreenDragStartPosition;
	// apply inverse parent transform to operate on delta in local space
	DragDelta = ParentTransform.Get(FTransform2D()).Inverse().TransformVector(DragDelta);

	// change active handle temporarily if holding shift (constrain to an axis)
	if (MouseEvent.IsShiftDown())
	{
		if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateXY)
		{
			if (FMath::Abs(DragDelta.X) > FMath::Abs(DragDelta.Y))
			{
				ActiveHandle = EMGFXShapeTransformHandle::TranslateX;
			}
			else if (FMath::Abs(DragDelta.X) < FMath::Abs(DragDelta.Y))
			{
				ActiveHandle = EMGFXShapeTransformHandle::TranslateY;
			}
		}
	}
	else if (ActiveHandle != OriginalActiveHandle)
	{
		// remove temporary axis constraint
		ActiveHandle = OriginalActiveHandle;
	}

	// apply axis constraints
	if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateX)
	{
		DragDelta *= FVector2D(1.f, 0.f);
	}
	else if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateY)
	{
		DragDelta *= FVector2D(0.f, 1.f);
	}

	// account for window dpi and application scale
	DragDelta /= GetDPIScale();

	// we're tracking the total delta for accuracy,
	// so add to the original location recorded on drag start
	const FVector2D OffsetLocation = OriginalLocation + DragDelta;

	// TODO: ideally hold X to snap
	// optionally snap to grid
	const bool bSnapToGrid = MouseEvent.GetModifierKeys().IsControlDown();
	const FVector2D NewLocation = bSnapToGrid ? SnapLocationToGrid(OffsetLocation) : OffsetLocation;

	// keep track of whether the new value is actually different...
	bWasModified = !(OriginalLocation - NewLocation).IsNearlyZero();

	// ...but always send the interactive change event
	OnSetLocationEvent.ExecuteIfBound(NewLocation, false);

	return FReply::Handled();
}

FReply SMGFXShapeTransformHandle::PerformMouseMoveRotate(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	check(LocalDragStartPosition.IsSet());

	const FVector2D StartDir = LocalDragStartPosition.GetValue().GetSafeNormal();
	const FVector2D NewDir = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).GetSafeNormal();

	const float OrigAngle = FMath::Acos(StartDir.X) * FMath::Sign(StartDir.Y);
	const float NewAngle = FMath::Acos(NewDir.X) * FMath::Sign(NewDir.Y);
	const float DeltaAngle = FMath::RadiansToDegrees(NewAngle - OrigAngle);
	const float OffsetAngle = OriginalRotation + DeltaAngle;

	// optionally snap to grid
	const bool bSnapToGrid = MouseEvent.GetModifierKeys().IsControlDown();
	const float NewRotation = FMath::UnwindDegrees(bSnapToGrid ? SnapRotationToGrid(OffsetAngle) : OffsetAngle);

	// keep track of whether the new value is actually different...
	bWasModified = !FMath::IsNearlyEqual(NewRotation, OriginalRotation);

	// ...but always send the interactive change event
	OnSetRotationEvent.ExecuteIfBound(NewRotation, false);

	return FReply::Handled();
}

FReply SMGFXShapeTransformHandle::PerformMouseMoveScale(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// the transform to apply to drag deltas so they operate in local space
	const float LocalRotation = FMath::DegreesToRadians(Rotation.Get());
	const FTransform2D HandleSpaceTransform = FTransform2D(FQuat2D(LocalRotation)).Concatenate(ParentTransform.Get(FTransform2D())).Inverse();

	const float DPIScale = GetDPIScale();

	// get the raw delta, using screen space coords
	FVector2D DragDelta = MouseEvent.GetScreenSpacePosition() - ScreenDragStartPosition;
	// the distance that must be dragged to add +/- 100% scale
	float ScaleUnitDistance = HandleLength;

	// true when temporarily constraining to a single axis by holding down shift
	const bool bWantsConstrainAxis = MouseEvent.IsShiftDown();
	// true if the delta scale should be the same for both X and Y
	const bool bKeepUniform = ActiveHandle.GetValue() == EMGFXShapeTransformHandle::ScaleXY && !bWantsConstrainAxis;

	if (!bKeepUniform)
	{
		// apply inverse parent transform to operate in local space
		DragDelta = HandleSpaceTransform.TransformVector(DragDelta);
		// scale unit should also now be in local space
		ScaleUnitDistance = HandleSpaceTransform.TransformVector(FVector2D(HandleLength, 0.f)).Length();
	}

	// flip Y since negative UI coordinates == positive scale
	DragDelta.Y *= -1.f;

	// change active handle temporarily if holding shift (constrain to an axis)
	if (bWantsConstrainAxis)
	{
		if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::ScaleXY)
		{
			if (FMath::Abs(DragDelta.X) > FMath::Abs(DragDelta.Y))
			{
				ActiveHandle = EMGFXShapeTransformHandle::ScaleX;
			}
			else if (FMath::Abs(DragDelta.X) < FMath::Abs(DragDelta.Y))
			{
				ActiveHandle = EMGFXShapeTransformHandle::ScaleY;
			}
		}
	}
	else if (ActiveHandle != OriginalActiveHandle)
	{
		// remove temporary axis constraint
		ActiveHandle = OriginalActiveHandle;
	}


	// apply axis constraints
	if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::ScaleX)
	{
		DragDelta *= FVector2D(1.f, 0.f);
	}
	else if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::ScaleY)
	{
		DragDelta *= FVector2D(0.f, 1.f);
	}
	else if (bKeepUniform)
	{
		// apply uniform scale constraint
		const float DragDeltaSum = (DragDelta.X + DragDelta.Y) / 2.f;
		DragDelta = FVector2D(DragDeltaSum, DragDeltaSum);
	}

	// account for window dpi and application scale
	DragDelta /= DPIScale;

	// dragging the same distance of the handle length will cause the scale to change by +/- 1.0
	const float ScaleSensitivity = 1.f / ScaleUnitDistance;
	const FVector2D OffsetScale = OriginalScale * (FVector2D::One() + DragDelta * ScaleSensitivity);

	// TODO: ideally hold X to snap
	// optionally snap to grid
	const bool bSnapToGrid = MouseEvent.GetModifierKeys().IsControlDown();
	const FVector2D NewScale = bSnapToGrid ? SnapScaleToGrid(OffsetScale) : OffsetScale;

	// keep track of whether the new value is actually different...
	bWasModified = !(OriginalScale - NewScale).IsNearlyZero();

	// ...but always send the interactive change event
	OnSetScaleEvent.ExecuteIfBound(NewScale, false);

	return FReply::Handled();
}

FVector2D SMGFXShapeTransformHandle::SnapLocationToGrid(FVector2D InLocation) const
{
	// TODO: expose
	const float GridSize = 4.f;
	return FVector2D(
		FMath::RoundHalfFromZero(InLocation.X / GridSize) * GridSize,
		FMath::RoundHalfFromZero(InLocation.Y / GridSize) * GridSize);
}

float SMGFXShapeTransformHandle::SnapRotationToGrid(float InRotation) const
{
	// TODO: expose
	const float RotationGridAngle = 15.f;
	return FMath::RoundHalfFromZero(InRotation / RotationGridAngle) * RotationGridAngle;
}

FVector2D SMGFXShapeTransformHandle::SnapScaleToGrid(FVector2D InScale) const
{
	// TODO: expose
	const float GridSize = 0.1f;
	return FVector2D(
		FMath::RoundHalfFromZero(InScale.X / GridSize) * GridSize,
		FMath::RoundHalfFromZero(InScale.Y / GridSize) * GridSize);
}

float SMGFXShapeTransformHandle::GetDPIScale() const
{
	float DPIScale = FSlateApplication::Get().GetApplicationScale();
	const TSharedPtr<SWindow> WidgetWindow = FSlateApplication::Get().FindWidgetWindow(this->AsShared());
	if (WidgetWindow.IsValid())
	{
		DPIScale *= WidgetWindow->GetNativeWindow()->GetDPIScaleFactor();
	}
	return DPIScale;
}

void SMGFXShapeTransformHandle::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SMGFXShapeTransformHandle::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	if (!bIsDragging)
	{
		ActiveHandle.Reset();
	}

	SCompoundWidget::OnMouseLeave(MouseEvent);
}

int32 SMGFXShapeTransformHandle::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                         FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// manually draw handles so they can mismatch the actual hit area of the handle SImages

	++LayerId;
	if (GetHandleVisibility(EMGFXShapeTransformHandle::TranslateX) == EVisibility::Visible)
	{
		const FVector2D Direction = FVector2D(1.f, 0.f);
		const FLinearColor Color = GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor();

		PaintTranslateHandle(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, Direction, Color);
	}

	if (GetHandleVisibility(EMGFXShapeTransformHandle::TranslateY) == EVisibility::Visible)
	{
		const FVector2D Direction = FVector2D(0.f, -1.f);
		const FLinearColor Color = GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor();

		PaintTranslateHandle(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, Direction, Color);
	}

	if (GetHandleVisibility(EMGFXShapeTransformHandle::Rotate) == EVisibility::Visible)
	{
		const FLinearColor Color = GetHandleColor(EMGFXShapeTransformHandle::Rotate).GetSpecifiedColor();
		PaintRotateHandle(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, Color);
	}

	if (GetHandleVisibility(EMGFXShapeTransformHandle::ScaleX) == EVisibility::Visible)
	{
		const FVector2D Direction = FVector2D(1.f, 0.f);
		const FLinearColor Color = GetHandleColor(EMGFXShapeTransformHandle::ScaleX).GetSpecifiedColor();

		PaintScaleHandle(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, Direction, Color);
	}
	if (GetHandleVisibility(EMGFXShapeTransformHandle::ScaleY) == EVisibility::Visible)
	{
		const FVector2D Direction = FVector2D(0.f, -1.f);
		const FLinearColor Color = GetHandleColor(EMGFXShapeTransformHandle::ScaleY).GetSpecifiedColor();

		PaintScaleHandle(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, Direction, Color);
	}

	return LayerId;
}

int32 SMGFXShapeTransformHandle::PaintTranslateHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Direction,
                                                      const FLinearColor& Color) const
{
	// transform to apply to handles so that they are rotated to match parent space
	const FTransform2D HandlesTransform = GetParentInverseRotationTransform();

	const FVector2D EndPoint = Direction * HandleLength;
	const FVector2D CrossDir = FVector2D(Direction.Y, -Direction.X);

	const TArray<FVector2D> LinePoints = {
		HandlesTransform.TransformPoint(FVector2D::ZeroVector),
		HandlesTransform.TransformPoint(EndPoint)
	};
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints,
	                             ESlateDrawEffect::None, Color, true, HandleLineWidth);

	// arrow head is 3x longer than it is wide, and centered on the end point
	const FVector2D ArrowBasePoint = EndPoint - Direction * ArrowSize / 2.f;
	const TArray<FVector2D> ArrowPoints = {
		HandlesTransform.TransformPoint(ArrowBasePoint + CrossDir * ArrowSize / 3.f),
		HandlesTransform.TransformPoint(ArrowBasePoint + Direction * ArrowSize),
		HandlesTransform.TransformPoint(ArrowBasePoint - CrossDir * ArrowSize / 3.f),
	};

	TArray<FSlateVertex> ArrowVertices;
	ArrowVertices.Reserve(3);
	for (int32 PointIndex = 0; PointIndex != 3; ++PointIndex)
	{
		ArrowVertices.AddZeroed();
		FSlateVertex& NewVert = ArrowVertices.Last();
		NewVert.Position = FVector2f(AllottedGeometry.LocalToAbsolute(ArrowPoints[PointIndex]));
		NewVert.Color = Color.ToFColor(false);
	}
	const TArray<SlateIndex> VertexIndices = {0, 1, 2};

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements, LayerId, FAppStyle::GetBrush("WhiteBrush")->GetRenderingResource(), ArrowVertices, VertexIndices, nullptr, 0, 0);

	return LayerId;
}

int32 SMGFXShapeTransformHandle::PaintRotateHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                                   int32 LayerId, const FLinearColor& Color) const
{
	// transform to apply to handles so that they represent the current rotation
	// (this is not just the parent transform)
	const FTransform2D HandlesTransform = GetInverseRotationTransform();

	const FVector2D Direction = FVector2D(1.f, 0.f) * HandleLength;
	constexpr int32 Resolution = 64;
	TArray<FVector2D> CirclePoints;

	for (int32 Idx = 0; Idx <= Resolution; ++Idx)
	{
		const float Angle = static_cast<float>(Idx) / Resolution * 360.f;
		const FVector2D CirclePoint = HandlesTransform.TransformPoint(Direction.GetRotated(Angle));
		CirclePoints.Add(CirclePoint);
	}

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CirclePoints,
	                             ESlateDrawEffect::None, Color, true, HandleLineWidth);

	// draw tick marks for X and Y axes
	++LayerId;
	const TArray<FVector2D> XTickPoints = {
		HandlesTransform.TransformPoint(FVector2D(HandleLength, 0.f) - FVector2D(5.f, 0.f)),
		HandlesTransform.TransformPoint(FVector2D(HandleLength, 0.f) + FVector2D(5.f, 0.f))
	};
	const FLinearColor XColor = GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor();
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), XTickPoints,
	                             ESlateDrawEffect::None, XColor, true, 10.f);

	const TArray<FVector2D> YTickPoints = {
		HandlesTransform.TransformPoint(FVector2D(0.f, -HandleLength) - FVector2D(0.f, 5.f)),
		HandlesTransform.TransformPoint(FVector2D(0.f, -HandleLength) + FVector2D(0.f, 5.f))
	};
	const FLinearColor YColor = GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor();
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), YTickPoints,
	                             ESlateDrawEffect::None, YColor, true, 10.f);

	// if dragging, visualize delta angle from X vector
	if (bIsDragging)
	{
		const float DeltaRotation = Rotation.Get() - OriginalRotation;

		// draw current rotations X vector
		const TArray<FVector2D> RotatedAnglePoints = {
			FVector2D::ZeroVector,
			HandlesTransform.TransformVector(FVector2D(HandleLength, 0.f))
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), RotatedAnglePoints,
		                             ESlateDrawEffect::None, FLinearColor::Yellow);

		// draw original rotations X vector
		const FTransform2D InvDeltaRotationTransform(FQuat2D(FMath::DegreesToRadians(-DeltaRotation)));
		const FTransform2D OrigHandlesTransform = InvDeltaRotationTransform.Concatenate(HandlesTransform);
		const TArray<FVector2D> OrigAnglePoints = {
			FVector2D::ZeroVector,
			OrigHandlesTransform.TransformVector(FVector2D(HandleLength, 0.f))
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), OrigAnglePoints,
		                             ESlateDrawEffect::None, FLinearColor::Yellow);
	}

	return LayerId;
}

int32 SMGFXShapeTransformHandle::PaintScaleHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                                  int32 LayerId, const FVector2D& Direction, const FLinearColor& Color) const
{
	// transform to apply to handles so that they represent the current rotation
	// (scale comes after rotation)
	const FTransform2D HandlesTransform = GetInverseRotationTransform();

	// if dragging, visualize the delta scale for this direction
	float EffectiveDeltaScale = 1.f;
	if (bIsDragging)
	{
		const FVector2D DeltaScale = Scale.Get() / OriginalScale;
		// inverse Y component since Y handle direction is negative, but that should align with positive scale
		const FVector2D InvDeltaScale = DeltaScale * FVector2D(1.f, -1.f);

		EffectiveDeltaScale = Direction.Dot(InvDeltaScale);
	}

	const FVector2D EndPoint = Direction * HandleLength * EffectiveDeltaScale;

	// draw line
	const TArray<FVector2D> LinePoints = {
		HandlesTransform.TransformPoint(FVector2D::ZeroVector),
		HandlesTransform.TransformPoint(EndPoint)
	};
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints,
	                             ESlateDrawEffect::None, Color, true, HandleLineWidth);

	// draw square
	const FVector2D BoxSize = FVector2D(ArrowSize, ArrowSize);
	const FVector2D BoxHalfSize = BoxSize / 2.f;
	const FSlateLayoutTransform BoxTransform(EndPoint - BoxHalfSize);
	const FPaintGeometry BoxGeometry = AllottedGeometry.MakeChild(HandlesTransform).ToPaintGeometry(BoxSize, BoxTransform);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, BoxGeometry,
	                           FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);

	return LayerId;
}

TOptional<FSlateRenderTransform> SMGFXShapeTransformHandle::GetHandlesRenderTransform(EMGFXShapeTransformHandle HandleType) const
{
	// just applying rotation, location is already set by the parent widget,
	// and scale should not be applied so that handle sizes are constant
	switch (HandleType)
	{
	default:
	case EMGFXShapeTransformHandle::TranslateX:
	case EMGFXShapeTransformHandle::TranslateY:
	case EMGFXShapeTransformHandle::TranslateXY:
		// represent only the parent's rotation, since T happens before RS
		return GetParentInverseRotationTransform();
	case EMGFXShapeTransformHandle::Rotate:
	case EMGFXShapeTransformHandle::ScaleX:
	case EMGFXShapeTransformHandle::ScaleY:
	case EMGFXShapeTransformHandle::ScaleXY:
		// represent the local rotation in both rotate and scale transform handles
		return GetInverseRotationTransform();
	}
}

FTransform2D SMGFXShapeTransformHandle::GetInverseRotationTransform() const
{
	const float ParentRotation = ParentTransform.Get().Inverse().GetMatrix().GetRotationAngle();
	const float LocalRotation = FMath::DegreesToRadians(Rotation.Get());
	return FTransform2D(FQuat2D(ParentRotation + LocalRotation));
}

FTransform2D SMGFXShapeTransformHandle::GetParentInverseRotationTransform() const
{
	const float ParentRotation = ParentTransform.Get().Inverse().GetMatrix().GetRotationAngle();
	return FTransform2D(FQuat2D(ParentRotation));
}

FSlateColor SMGFXShapeTransformHandle::GetHandleColor(EMGFXShapeTransformHandle HandleType) const
{
	if (ActiveHandle.IsSet() && IsMatchingOrRelevantHandle(HandleType, ActiveHandle.GetValue()))
	{
		return FLinearColor::Yellow;
	}

	switch (HandleType)
	{
	case EMGFXShapeTransformHandle::TranslateX:
		return FLinearColor::Red;
		break;
	case EMGFXShapeTransformHandle::TranslateY:
		return FLinearColor::Green;
		break;
	case EMGFXShapeTransformHandle::TranslateXY:
		return FLinearColor::Blue;
		break;
	case EMGFXShapeTransformHandle::Rotate:
		return FMath::Lerp(FLinearColor::Yellow, FLinearColor::White, 0.5f);
		break;
	case EMGFXShapeTransformHandle::ScaleX:
		return FLinearColor::Red;
		break;
	case EMGFXShapeTransformHandle::ScaleY:
		return FLinearColor::Green;
		break;
	case EMGFXShapeTransformHandle::ScaleXY:
		return FLinearColor::Blue;
		break;
	default: ;
	}

	return FLinearColor::White;
}

EVisibility SMGFXShapeTransformHandle::GetHandleVisibility(EMGFXShapeTransformHandle HandleType) const
{
	if (bIsDragging)
	{
		// hide all handles but the one being used
		return IsMatchingOrRelevantHandle(HandleType, ActiveHandle.GetValue()) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	switch (HandleType)
	{
	case EMGFXShapeTransformHandle::TranslateX:
	case EMGFXShapeTransformHandle::TranslateY:
	case EMGFXShapeTransformHandle::TranslateXY:
		return Mode.Get() == EMGFXShapeTransformMode::Translate ? EVisibility::Visible : EVisibility::Collapsed;
		break;
	case EMGFXShapeTransformHandle::Rotate:
		return Mode.Get() == EMGFXShapeTransformMode::Rotate ? EVisibility::Visible : EVisibility::Collapsed;
		break;
	case EMGFXShapeTransformHandle::ScaleX:
	case EMGFXShapeTransformHandle::ScaleY:
	case EMGFXShapeTransformHandle::ScaleXY:
		return Mode.Get() == EMGFXShapeTransformMode::Scale ? EVisibility::Visible : EVisibility::Collapsed;
		break;
	default: ;
	}
	return EVisibility::Visible;
}

bool SMGFXShapeTransformHandle::IsMatchingOrRelevantHandle(EMGFXShapeTransformHandle HandleType, EMGFXShapeTransformHandle OtherHandleType) const
{
	if (HandleType == OtherHandleType)
	{
		return true;
	}
	else if (OtherHandleType == EMGFXShapeTransformHandle::TranslateXY)
	{
		return HandleType == EMGFXShapeTransformHandle::TranslateX ||
			HandleType == EMGFXShapeTransformHandle::TranslateY;
	}
	else if (OtherHandleType == EMGFXShapeTransformHandle::ScaleXY)
	{
		return HandleType == EMGFXShapeTransformHandle::ScaleX ||
			HandleType == EMGFXShapeTransformHandle::ScaleY;
	}
	return false;
}

TOptional<EMGFXShapeTransformHandle> SMGFXShapeTransformHandle::GetHoveredHandle() const
{
	if (TranslateXHandle->IsHovered())
	{
		return EMGFXShapeTransformHandle::TranslateX;
	}
	else if (TranslateYHandle->IsHovered())
	{
		return EMGFXShapeTransformHandle::TranslateY;
	}
	else if (RotateHandle->IsHovered())
	{
		return EMGFXShapeTransformHandle::Rotate;
	}
	else if (ScaleXHandle->IsHovered())
	{
		return EMGFXShapeTransformHandle::ScaleX;
	}
	else if (ScaleYHandle->IsHovered())
	{
		return EMGFXShapeTransformHandle::ScaleY;
	}

	return TOptional<EMGFXShapeTransformHandle>();
}

FReply SMGFXShapeTransformHandle::StartDragging(const FPointerEvent& MouseEvent)
{
	switch (Mode.Get())
	{
	default:
	case EMGFXShapeTransformMode::Select:
		return FReply::Unhandled();
		break;
	case EMGFXShapeTransformMode::Translate:
		// multi axis translate
		return StartDragging(EMGFXShapeTransformHandle::TranslateXY, MouseEvent);
		break;
	case EMGFXShapeTransformMode::Rotate:
		return StartDragging(EMGFXShapeTransformHandle::Rotate, MouseEvent);
		break;
	case EMGFXShapeTransformMode::Scale:
		// multi axis scale
		return StartDragging(EMGFXShapeTransformHandle::ScaleXY, MouseEvent);
		break;
	}
}


FReply SMGFXShapeTransformHandle::StartDragging(EMGFXShapeTransformHandle HandleType, const FPointerEvent& MouseEvent)
{
	bIsDragging = true;
	bWasModified = false;

	OriginalLocation = Location.Get();
	OriginalRotation = Rotation.Get();
	OriginalScale = Scale.Get();

	ActiveHandle = HandleType;
	OriginalActiveHandle = ActiveHandle;

	ScreenDragStartPosition = MouseEvent.GetScreenSpacePosition();
	// set local drag start on the next mouse move, when we have geometry
	LocalDragStartPosition.Reset();

	// TODO: expose as option, could be moving something other than a 'layer'
	Transaction = new FScopedTransaction(NSLOCTEXT("MGFXMaterialEditor", "MoveLayer", "Move Layer"));

	OnDragStartedEvent.ExecuteIfBound();

	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SMGFXShapeTransformHandle::FinishDragging()
{
	// broadcast setter events with bIsFinished set to true
	switch (Mode.Get())
	{
	default: ;
	case EMGFXShapeTransformMode::Select:
		break;
	case EMGFXShapeTransformMode::Translate:
		OnSetLocationEvent.ExecuteIfBound(Location.Get(), true);
		break;
	case EMGFXShapeTransformMode::Rotate:
		OnSetRotationEvent.ExecuteIfBound(Rotation.Get(), true);
		break;
	case EMGFXShapeTransformMode::Scale:
		OnSetScaleEvent.ExecuteIfBound(Scale.Get(), true);
		break;
	}

	OnDragFinishedEvent.ExecuteIfBound(bWasModified);

	// complete the undo transaction
	if (Transaction)
	{
		if (!bWasModified)
		{
			Transaction->Cancel();
		}

		delete Transaction;
		Transaction = nullptr;
	}

	// update active handle, which could be nothing now
	ActiveHandle = GetHoveredHandle();
	OriginalActiveHandle.Reset();
	LocalDragStartPosition.Reset();

	bIsDragging = false;

	return FReply::Handled().ReleaseMouseCapture();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
