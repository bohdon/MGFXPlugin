// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXShapeTransformHandle.h"

#include "MGFXEditorModule.h"
#include "MGFXEditorStyle.h"
#include "SlateOptMacros.h"
#include "Widgets/SCanvas.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXShapeTransformHandle::SMGFXShapeTransformHandle()
	: Transaction(nullptr)
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
	const float HandleLength = InArgs._HandleLength;
	const float HandleWidth = InArgs._HandleWidth;

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
			.Image(FMGFXEditorStyle::Get().GetBrush("NoBrush"))
			.ColorAndOpacity(FLinearColor::Transparent)
			// .ColorAndOpacity(this, &SMGFXShapeTransformHandle::GetHandleColor, EMGFXShapeTransformHandle::TranslateX)
		]

		// Y-axis
		+ SCanvas::Slot()
		  .Position(FVector2D(0.f, -HandleHalfWidth))
		  .Size(FVector2D(HandleWidth, HandleLength))
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SAssignNew(TranslateYHandle, SImage)
			.Image(FMGFXEditorStyle::Get().GetBrush("NoBrush"))
			.ColorAndOpacity(FLinearColor::Transparent)
			// .ColorAndOpacity(this, &SMGFXShapeTransformHandle::GetHandleColor, EMGFXShapeTransformHandle::TranslateY)
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
		check(ActiveHandle.IsSet());

		// TODO: perform drag
		FVector2D DragDelta = MouseEvent.GetScreenSpacePosition() - DragStartPosition;

		// clamp to axis
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

		UE_LOG(LogTemp, Log, TEXT("Move: %s"), *DragDelta.ToString());

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
	UE_LOG(LogTemp, Log, TEXT("Over: "));


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
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{FVector2D(0.f, 0.f), FVector2D(100.f, 0.f)},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateX).GetSpecifiedColor()
		);
	}

	if (GetHandleVisibility(EMGFXShapeTransformHandle::TranslateY) == EVisibility::Visible)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{FVector2D(0.f, 0.f), FVector2D(0.f, -100.f)},
			ESlateDrawEffect::None,
			GetHandleColor(EMGFXShapeTransformHandle::TranslateY).GetSpecifiedColor()
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

	bIsDragging = false;

	return FReply::Handled().ReleaseMouseCapture();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
