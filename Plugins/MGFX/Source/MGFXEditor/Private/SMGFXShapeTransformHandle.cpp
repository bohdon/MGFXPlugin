// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXShapeTransformHandle.h"

#include "MGFXEditorModule.h"
#include "MGFXEditorStyle.h"
#include "SlateOptMacros.h"
#include "Widgets/SCanvas.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXShapeTransformHandle::SMGFXShapeTransformHandle()
	: ArrowSize(7.f),
	  Transaction(nullptr)
{
}

void SMGFXShapeTransformHandle::Construct(const FArguments& InArgs)
{
	if (!InArgs._OnGetTransform.IsBound())
	{
		UE_LOG(LogMGFXEditor, Error, TEXT("OnGetTransform must be set."))
	}

	OnGetTransform = InArgs._OnGetTransform;
	OnMoveTransform = InArgs._OnMoveTransform;
	OnDragFinished = InArgs._OnDragFinished;
	HandleLength = InArgs._HandleLength;
	HandleWidth = InArgs._HandleWidth;
	HandleLineWidth = InArgs._HandleLineWidth;
	ArrowSize = InArgs._ArrowSize;

	const float HandleHalfWidth = HandleWidth / 2.f;

	// handle SImages are for hit testing only, lines are drawn in OnPaint that are thinner and more appealing

	ChildSlot
	[
		SNew(SCanvas)

		// translate manipulator
		// X-axis
		+ SCanvas::Slot()
		  .Position(FVector2D(HandleHalfWidth, 0.f))
		  .Size(FVector2D(HandleLength, HandleWidth))
		  .HAlign(HAlign_Left)
		  .VAlign(VAlign_Center)
		[
			SAssignNew(TranslateXHandle, SImage)
			.ColorAndOpacity(FLinearColor::Transparent)
		]

		// Y-axis
		+ SCanvas::Slot()
		  .Position(FVector2D(0.f, -HandleHalfWidth))
		  .Size(FVector2D(HandleWidth, HandleLength))
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SAssignNew(TranslateYHandle, SImage)
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

		// get the raw delta
		FVector2D DragDelta = MouseEvent.GetScreenSpacePosition() - DragStartPosition;

		// temporarily change active handle if holding certain modifiers
		ActiveHandle = OriginalActiveHandle;
		if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateXY && MouseEvent.IsShiftDown())
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

		// clamp delta to axis
		const bool bIsShiftConstrainingAxis = MouseEvent.IsShiftDown() && ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateXY;

		if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateX)
		{
			DragDelta *= FVector2D(1.f, 0.f);
		}
		else if (ActiveHandle.GetValue() == EMGFXShapeTransformHandle::TranslateY)
		{
			DragDelta *= FVector2D(0.f, 1.f);
		}

		const FTransform2D DeltaTransform = FTransform2D(DragDelta);
		const FTransform2D NewTransform = DragStartTransform.Concatenate(DeltaTransform);

		OnMoveTransform.ExecuteIfBound(NewTransform);
	}
	else
	{
		ActiveHandle = GetHoveredHandle();
	}

	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
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
		const FVector2D EndPoint = FVector2D(100.f, 0.f);
		const FVector2D Direction = FVector2D(1.f, 0.f);
		const FVector2D Up = FVector2D(0.f, 1.f);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{FVector2D::ZeroVector, EndPoint},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor(),
			true,
			HandleLineWidth
		);
		// arrow head
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{EndPoint - Direction * ArrowSize + Up * ArrowSize, EndPoint, EndPoint - Direction * ArrowSize - Up * ArrowSize},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor(),
			true,
			HandleLineWidth
		);
	}

	if (GetHandleVisibility(EMGFXShapeTransformHandle::TranslateY) == EVisibility::Visible)
	{
		const FVector2D EndPoint = FVector2D(0.f, -100.f);
		const FVector2D Direction = FVector2D(0.f, -1.f);
		const FVector2D Up = FVector2D(1.f, 0.f);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{FVector2D::ZeroVector, EndPoint},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor(),
			true,
			HandleLineWidth
		);
		// arrow head
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{EndPoint - Direction * ArrowSize + Up * ArrowSize, EndPoint, EndPoint - Direction * ArrowSize - Up * ArrowSize},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor(),
			true,
			HandleLineWidth
		);
	}

	return LayerId;
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

	return TOptional<EMGFXShapeTransformHandle>();
}


FReply SMGFXShapeTransformHandle::StartDragging(EMGFXShapeTransformHandle HandleType, const FPointerEvent& MouseEvent)
{
	bIsDragging = true;
	ActiveHandle = HandleType;
	OriginalActiveHandle = ActiveHandle;

	DragStartPosition = MouseEvent.GetScreenSpacePosition();
	DragStartTransform = OnGetTransform.Execute();

	// TODO: expose as option, could be using this to move other things
	Transaction = new FScopedTransaction(NSLOCTEXT("MGFXMaterialEditor", "MoveLayer", "Move Layer"));

	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SMGFXShapeTransformHandle::FinishDragging()
{
	// commit the operation, allowing object modification etc
	OnDragFinished.ExecuteIfBound();

	// complete the undo transaction
	if (Transaction)
	{
		delete Transaction;
		Transaction = nullptr;
	}

	// update active handle, which could be nothing now
	ActiveHandle = GetHoveredHandle();
	OriginalActiveHandle.Reset();

	bIsDragging = false;

	return FReply::Handled().ReleaseMouseCapture();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
