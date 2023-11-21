// Copyright Bohdon Sayre, All Rights Reserved.


#include "SArtboardPanel.h"

#include "MGFXEditorStyle.h"
#include "SlateOptMacros.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


// SArtboardPanel::FSlot
// ---------------------

void SArtboardPanel::FSlot::Construct(const FChildren& SlotOwner, FSlotArguments&& InArg)
{
	TWidgetSlotWithAttributeSupport<FSlot>::Construct(SlotOwner, MoveTemp(InArg));

	if (InArg._Size.IsSet())
	{
		Size.Assign(*this, MoveTemp(InArg._Size));
	}
	if (InArg._Position.IsSet())
	{
		Position.Assign(*this, MoveTemp(InArg._Position));
	}
}

void SArtboardPanel::FSlot::RegisterAttributes(FSlateWidgetSlotAttributeInitializer& AttributeInitializer)
{
}


// SArtboardPanel
// --------------

SArtboardPanel::SArtboardPanel()
	: ViewOffset(FVector2D::ZeroVector),
	  ViewOffsetMargin(FVector2D(100.f, 100.f)),
	  ZoomAmount(1.f),
	  ZoomAmountMin(0.1f),
	  ZoomAmountMax(10.f),
	  bIsPanning(false),
	  bIsZooming(false),
	  ZoomSensitivity(0.001f),
	  ZoomWheelSensitivity(0.1f),
	  Children(this, GET_MEMBER_NAME_CHECKED(SArtboardPanel, Children)),
	  bIsViewOffsetInitialized(false)
{
}

void SArtboardPanel::Construct(const FArguments& InArgs)
{
	ArtboardSize = InArgs._ArtboardSize;
	ZoomAmountMin = InArgs._ZoomAmountMin;
	ZoomAmountMax = InArgs._ZoomAmountMax;
	BackgroundBrush = InArgs._BackgroundBrush;
	bShowArtboardBorder = InArgs._bShowArtboardBorder;
	OnViewOffsetChangedEvent = InArgs._OnViewOffsetChanged;
	OnZoomChangedEvent = InArgs._OnZoomChanged;

	Children.AddSlots(MoveTemp(const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots)));
}

TSharedRef<SWidget> SArtboardPanel::ConstructOverlay()
{
	return SNew(SOverlay);
}

void SArtboardPanel::ClearChildren()
{
	Children.Empty();
}

SArtboardPanel::FSlot::FSlotArguments SArtboardPanel::Slot()
{
	return FSlot::FSlotArguments(MakeUnique<FSlot>());
}

SArtboardPanel::FSlot* SArtboardPanel::GetWidgetSlot(const TSharedPtr<SWidget>& Widget)
{
	for (int32 Idx = 0; Idx < Children.Num(); ++Idx)
	{
		if (Children[Idx].GetWidget() == Widget)
		{
			return &Children[Idx];
		}
	}
	return nullptr;
}

FChildren* SArtboardPanel::GetChildren()
{
	return &Children;
}

void SArtboardPanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	for (int32 Idx = 0; Idx < Children.Num(); ++Idx)
	{
		const FSlot& Child = Children[Idx];
		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Child.GetWidget(), Child.GetPosition() - ViewOffset, Child.GetSize(), ZoomAmount));
	}
}

FVector2D SArtboardPanel::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// avoid costly calculation, only support explicitly filling or manually sizing this widget
	return FVector2D(100.f, 100.f);
}

FCursorReply SArtboardPanel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// use software cursor when panning and zooming
	return ShouldShowSoftwareCursor() ? FCursorReply::Cursor(EMouseCursor::None) : FCursorReply::Cursor(EMouseCursor::Default);
}

int32 SArtboardPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                              FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = PaintBackground(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	++LayerId;
	LayerId = SPanel::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	LayerId = PaintBorder(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	LayerId = PaintSoftwareCursor(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);

	return LayerId;
}

FReply SArtboardPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SPanel::OnMouseButtonDown(MyGeometry, MouseEvent);

	FReply Reply = FReply::Unhandled();

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = false;
		bIsZooming = false;

		Reply.CaptureMouse(SharedThis(this));
	}

	return Reply;
}

FReply SArtboardPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SPanel::OnMouseButtonUp(MyGeometry, MouseEvent);

	if (!MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) &&
		!MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
	{
		bIsPanning = false;
		bIsZooming = false;

		FReply Reply = FReply::Handled();
		Reply.ReleaseMouseCapture();
		return Reply;
	}

	return FReply::Unhandled();
}

FReply SArtboardPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	SoftwareCursorPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	const bool bIsRMBDown = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const bool bIsLMBDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
	const bool bIsMMBDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	const FModifierKeysState ModifierKeysState = FSlateApplication::Get().GetModifierKeys();

	const bool bShouldZoom = bIsRMBDown && (bIsLMBDown || bIsMMBDown || ModifierKeysState.IsAltDown() || FSlateApplication::Get().IsUsingTrackpad());
	if (bShouldZoom)
	{
		const FVector2D CursorDelta = MouseEvent.GetCursorDelta();

		const float ZoomDelta = (CursorDelta.X + CursorDelta.Y) * ZoomSensitivity;

		// remove mouse movement that's been 'used up' by zooming
		if (ZoomDelta != 0)
		{
			if (!bIsZooming)
			{
				// initialize zoom coordinates
				ZoomFocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetLastScreenSpacePosition());
			}

			bIsZooming = true;
			bIsPanning = false;

			ApplyZoomDelta(ZoomDelta, ZoomFocalPosition, MyGeometry);
		}

		return FReply::Handled();
	}

	if (bIsRMBDown || bIsMMBDown)
	{
		if (!bIsPanning)
		{
			// initialize pan coordinates
			PanViewOffsetStart = ViewOffset;
			PanStartPosition = MouseEvent.GetLastScreenSpacePosition();
		}

		bIsPanning = true;
		bIsZooming = false;

		const FVector2D DeltaViewOffset = (PanStartPosition - MouseEvent.GetScreenSpacePosition()) / MyGeometry.Scale / GetZoomAmount();
		const FVector2D NewViewOffset = PanViewOffsetStart + DeltaViewOffset;
		SetViewOffset(NewViewOffset, MyGeometry);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SArtboardPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// zoom into the focus point; i.e. keep it the same fraction offset in the panel
	const FVector2D FocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float ZoomDelta = MouseEvent.GetWheelDelta() * ZoomWheelSensitivity;
	ApplyZoomDelta(ZoomDelta, FocalPosition, MyGeometry);

	return FReply::Handled();
}

void SArtboardPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPanel::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!bIsViewOffsetInitialized)
	{
		bIsViewOffsetInitialized = true;

		CenterView(AllottedGeometry);
	}
}

int32 SArtboardPanel::PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* Background = BackgroundBrush.Get();
	if (!Background)
	{
		Background = FMGFXEditorStyle::Get().GetBrush(TEXT("ArtboardBackground"));
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Background,
		ESlateDrawEffect::None,
		Background->TintColor.GetSpecifiedColor()
	);

	return LayerId;
}

int32 SArtboardPanel::PaintBorder(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (bShowArtboardBorder.Get())
	{
		constexpr FLinearColor BorderColor = FLinearColor(1.f, 1.f, 1.f, 0.2f);
		const FVector2D ArtboardSizeVal = ArtboardSize.Get();
		const TArray<FVector2D> BorderPoints = {
			GraphCoordToPanelCoord(FVector2D(0, 0) * ArtboardSizeVal) + FVector2D(-2, -2),
			GraphCoordToPanelCoord(FVector2D(1, 0) * ArtboardSizeVal) + FVector2D(2, -2),
			GraphCoordToPanelCoord(FVector2D(1, 1) * ArtboardSizeVal) + FVector2D(2, 2),
			GraphCoordToPanelCoord(FVector2D(0, 1) * ArtboardSizeVal) + FVector2D(-2, 2),
			GraphCoordToPanelCoord(FVector2D(0, 0)) + FVector2D(-2, -2),
		};
		constexpr float BorderThickness = 1.f;

		++LayerId;
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			BorderPoints,
			ESlateDrawEffect::None,
			BorderColor,
			true,
			BorderThickness
		);
	}

	return LayerId;
}

int32 SArtboardPanel::PaintSoftwareCursor(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                          FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!ShouldShowSoftwareCursor())
	{
		return LayerId;
	}

	// Get appropriate software cursor, depending on whether we're panning or zooming
	const FSlateBrush* Brush = FAppStyle::GetBrush(bIsPanning ? TEXT("SoftwareCursor_Grab") : TEXT("SoftwareCursor_UpDown"));

	++LayerId;
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(
			Brush->ImageSize,
			FSlateLayoutTransform(
				SoftwareCursorPosition - (Brush->ImageSize / 2)
			)
		),
		Brush
	);

	return LayerId;
}

bool SArtboardPanel::ShouldShowSoftwareCursor() const
{
	return bIsPanning || bIsZooming;
}


void SArtboardPanel::SetViewOffset(FVector2D NewViewOffset, const FGeometry& AllottedGeometry)
{
	// min is the furthest to the bottom-right allowed, which accounts for the widget geometry and current zoom
	const FVector2D ViewOffsetMin = (-AllottedGeometry.GetLocalSize() + ViewOffsetMargin) / GetZoomAmount();
	// max is based only on artboard size, since 0,0 coordinates are top-left
	const FVector2D ViewOffsetMax = ArtboardSize.Get() - (ViewOffsetMargin / GetZoomAmount());
	const FVector2D ClampedViewOffset = FVector2D(
		FMath::Clamp(NewViewOffset.X, ViewOffsetMin.X, ViewOffsetMax.X),
		FMath::Clamp(NewViewOffset.Y, ViewOffsetMin.Y, ViewOffsetMax.Y));

	if (ViewOffset != ClampedViewOffset)
	{
		ViewOffset = ClampedViewOffset;

		OnViewOffsetChanged();
	}
}

void SArtboardPanel::CenterView(const FGeometry& AllottedGeometry)
{
	const FVector2D NewViewOffset = -AllottedGeometry.GetLocalSize() * 0.5f + (ArtboardSize.Get() * 0.5f);
	SetViewOffset(NewViewOffset, AllottedGeometry);
}

void SArtboardPanel::SetZoomAmount(float NewZoomAmount)
{
	const float ClampedZoomAmount = FMath::Clamp(NewZoomAmount, ZoomAmountMin, ZoomAmountMax);
	if (ZoomAmount != ClampedZoomAmount)
	{
		ZoomAmount = ClampedZoomAmount;

		OnZoomChanged();
	}
}

void SArtboardPanel::ApplyZoomDelta(float ZoomDelta, const FVector2D& LocalFocalPosition, const FGeometry& AllottedGeometry)
{
	// we want to zoom into this point; i.e. keep it the same fraction offset into the panel
	const FVector2D GraphFocalPosition = PanelCoordToGraphCoord(LocalFocalPosition);

	// instead of just adding, apply proportional so 'zoom speed' is the same at all zoom levels
	SetZoomAmount(ZoomAmount * (1.f + ZoomDelta));

	// adjust offset to zoom around the target position
	const FVector2D NewViewOffset = GraphFocalPosition - LocalFocalPosition / GetZoomAmount();
	SetViewOffset(NewViewOffset, AllottedGeometry);
}

FVector2D SArtboardPanel::PanelCoordToGraphCoord(const FVector2D& PanelPosition) const
{
	return PanelPosition / GetZoomAmount() + GetViewOffset();
}

FVector2D SArtboardPanel::GraphCoordToPanelCoord(const FVector2D& GraphPosition) const
{
	return (GraphPosition - GetViewOffset()) * GetZoomAmount();
}

FTransform2D SArtboardPanel::GetPanelToGraphTransform() const
{
	return FTransform2D(ZoomAmount, -ViewOffset * ZoomAmount);
}

FTransform2D SArtboardPanel::GetGraphToPanelTransform() const
{
	return GetPanelToGraphTransform().Inverse();
}

void SArtboardPanel::OnViewOffsetChanged()
{
	OnViewOffsetChangedEvent.ExecuteIfBound(ViewOffset);
}

void SArtboardPanel::OnZoomChanged()
{
	OnZoomChangedEvent.ExecuteIfBound(ZoomAmount);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
