// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXShapeTransformHandle.h"

#include "MGFXEditorModule.h"
#include "SlateOptMacros.h"
#include "Widgets/SCanvas.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXShapeTransformHandle::SMGFXShapeTransformHandle()
	: HandleLength(80.f),
	  HandleWidth(14.f),
	  HandleLineWidth(2.f),
	  ArrowSize(14.f),
	  Transaction(nullptr)
{
}

void SMGFXShapeTransformHandle::Construct(const FArguments& InArgs)
{
	if (!InArgs._OnGetTransform.IsBound())
	{
		UE_LOG(LogMGFXEditor, Error, TEXT("OnGetTransform must be set."))
	}

	Mode = InArgs._Mode;
	OnGetTransform = InArgs._OnGetTransform;
	OnMoveTransform = InArgs._OnMoveTransform;
	OnDragFinished = InArgs._OnDragFinished;
	HandleLength = InArgs._HandleLength;
	HandleWidth = InArgs._HandleWidth;
	HandleLineWidth = InArgs._HandleLineWidth;
	ArrowSize = InArgs._ArrowSize;

	// handle SImages are for hit testing only, lines are drawn in OnPaint that are thinner and more appealing

	ChildSlot
	[
		SNew(SCanvas)

		// translate manipulator

		// Y-axis
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleWidth, HandleLength + ArrowSize))
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SAssignNew(TranslateYHandle, SImage)
			.ColorAndOpacity(FLinearColor::Transparent)
		]

		// X-axis
		+ SCanvas::Slot()
		  .Size(FVector2D(HandleLength + ArrowSize, HandleWidth))
		  .HAlign(HAlign_Left)
		  .VAlign(VAlign_Center)
		[
			SAssignNew(TranslateXHandle, SImage)
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

		// active handle can change temporarily when holding shift to constrain to an axis
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

	return LayerId;
}

int32 SMGFXShapeTransformHandle::PaintTranslateHandle(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Direction,
                                                      const FLinearColor& Color) const
{
	const FVector2D EndPoint = Direction * HandleLength;
	const FVector2D CrossDir = FVector2D(Direction.Y, -Direction.X);

	const TArray<FVector2D> LinePoints = {FVector2D::ZeroVector, EndPoint};
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints,
	                             ESlateDrawEffect::None, Color, true, HandleLineWidth);

	// arrow head is 3x longer than it is wide
	const TArray<FVector2D> ArrowPoints = {
		EndPoint + CrossDir * ArrowSize / 3.f,
		EndPoint + Direction * ArrowSize,
		EndPoint - CrossDir * ArrowSize / 3.f,
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
	const FVector2D Direction = FVector2D(1.f, 0.f) * HandleLength;
	constexpr int32 Resolution = 64;
	TArray<FVector2D> CirclePoints;

	for (int32 Idx = 0; Idx <= Resolution; ++Idx)
	{
		const float Angle = static_cast<float>(Idx) / Resolution * 360.f;
		CirclePoints.Add(Direction.GetRotated(Angle));
	}

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CirclePoints,
	                             ESlateDrawEffect::None, Color, true, HandleLineWidth);

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
