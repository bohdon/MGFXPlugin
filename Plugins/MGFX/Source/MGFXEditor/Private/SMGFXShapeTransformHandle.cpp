﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXShapeTransformHandle.h"

#include "MGFXEditorModule.h"
#include "SlateOptMacros.h"
#include "Widgets/SCanvas.h"


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
		.RenderTransform(this, &SMGFXShapeTransformHandle::GetHandlesRenderTransform)

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
	];
}

FReply SMGFXShapeTransformHandle::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// determine which handle to drag
	const TOptional<EMGFXShapeTransformHandle> HandleType = GetHoveredHandle();

	// start dragging
	if (HandleType.IsSet())
	{
		return StartDragging(HandleType.GetValue(), MouseEvent);
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
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
			// snap at the first sign of a direction when we haven't yet
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
	const bool bIsShiftConstrainingAxis = MouseEvent.IsShiftDown() && ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateXY;
	if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateX)
	{
		DragDelta *= FVector2D(1.f, 0.f);
	}
	else if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateY)
	{
		DragDelta *= FVector2D(0.f, 1.f);
	}
	// we're tracking the total delta for accuracy,
	// so add to the original location recorded on drag start
	const FVector2D OffsetLocation = OriginalLocation + DragDelta;

	// TODO: ideally hold X to snap
	// optionally snap to grid
	const bool bSnapToGrid = MouseEvent.GetModifierKeys().IsControlDown();
	const FVector2D NewLocation = bSnapToGrid ? SnapLocationToGrid(OffsetLocation) : OffsetLocation;

	// keep track of whether the new location is actually different...
	bWasModified = !(OriginalLocation - NewLocation).IsNearlyZero();

	// ...but always send the interactive change event
	OnSetLocationEvent.ExecuteIfBound(NewLocation);

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

	// keep track of whether the new rotation is actually different...
	bWasModified = !FMath::IsNearlyEqual(NewRotation, OriginalRotation);

	// ...but always send the interactive change event
	OnSetRotationEvent.ExecuteIfBound(NewRotation);

	return FReply::Handled();
}

FReply SMGFXShapeTransformHandle::PerformMouseMoveScale(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// const FVector2D NewScale = FVector2D::One();
	// OnSetScaleEvent.ExecuteIfBound(NewScale);
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

		// paint delta angle indicators while dragging
		if (bIsDragging && LocalDragStartPosition.IsSet())
		{
			const TArray<FVector2D> OrigAnglePoints = {
				FVector2D::ZeroVector, LocalDragStartPosition.GetValue().GetSafeNormal() * HandleLength
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), OrigAnglePoints,
			                             ESlateDrawEffect::None, FLinearColor::Yellow);

			const TArray<FVector2D> NewAnglePoints = {
				FVector2D::ZeroVector, LocalDragPosition.GetSafeNormal() * HandleLength
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), NewAnglePoints,
			                             ESlateDrawEffect::None, FLinearColor::Yellow);
		}
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

	// arrow head is 3x longer than it is wide
	const TArray<FVector2D> ArrowPoints = {
		HandlesTransform.TransformPoint(EndPoint + CrossDir * ArrowSize / 3.f),
		HandlesTransform.TransformPoint(EndPoint + Direction * ArrowSize),
		HandlesTransform.TransformPoint(EndPoint - CrossDir * ArrowSize / 3.f),
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
	TArray<FVector2D> XTickPoints = {
		HandlesTransform.TransformPoint(FVector2D(1.f, 0.f) * HandleLength - FVector2D(5.f, 0.f)),
		HandlesTransform.TransformPoint(FVector2D(1.f, 0.f) * HandleLength + FVector2D(5.f, 0.f))
	};
	const FLinearColor XColor = GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor();
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), XTickPoints,
	                             ESlateDrawEffect::None, XColor, true, 10.f);

	TArray<FVector2D> YTickPoints = {
		HandlesTransform.TransformPoint(FVector2D(0.f, -1.f) * HandleLength - FVector2D(0.f, 5.f)),
		HandlesTransform.TransformPoint(FVector2D(0.f, -1.f) * HandleLength + FVector2D(0.f, 5.f))
	};
	const FLinearColor YColor = GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor();
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), YTickPoints,
	                             ESlateDrawEffect::None, YColor, true, 10.f);

	return LayerId;
}

TOptional<FSlateRenderTransform> SMGFXShapeTransformHandle::GetHandlesRenderTransform() const
{
	// just applying rotation, location is already set by the parent widget,
	// and scale should not be applied so that handle sizes are constant
	return GetParentInverseRotationTransform();
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
