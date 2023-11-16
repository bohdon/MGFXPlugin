// Copyright Bohdon Sayre, All Rights Reserved.


#include "SArtboardPanel.h"

#include "SlateOptMacros.h"
#include "Settings/EditorStyleSettings.h"

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
	  DragDeltaDistance(0.f),
	  DragDeltaXY(0.f),
	  ZoomSensitivity(0.001f),
	  ZoomWheelSensitivity(0.1f),
	  Children(this, GET_MEMBER_NAME_CHECKED(SArtboardPanel, Children))
{
}

void SArtboardPanel::Construct(const FArguments& InArgs)
{
	ArtboardBounds.Max = InArgs._ArtboardSize.Get();
	BackgroundBrush = InArgs._BackgroundBrush;
	bShowArtboardBorder = InArgs._bShowArtboardBorder;

	if (!(BackgroundBrush.IsSet() || BackgroundBrush.IsBound()))
	{
		const FSlateBrush* DefaultBackground = FAppStyle::GetBrush(TEXT("Graph.Panel.SolidBackground"));
		const FSlateBrush* CustomBackground = &GetDefault<UEditorStyleSettings>()->GraphBackgroundBrush;
		BackgroundBrush.Set(CustomBackground->HasUObject() ? *CustomBackground : *DefaultBackground);
	}

	Children.AddSlots(MoveTemp(const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots)));
}

TSharedRef<SWidget> SArtboardPanel::ConstructOverlay()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		  .HAlign(HAlign_Left)
		  .VAlign(VAlign_Bottom)
		[
			SNew(STextBlock)
			.Text(this, &SArtboardPanel::GetDebugText)
		];
}

SArtboardPanel::FSlot::FSlotArguments SArtboardPanel::Slot()
{
	return FSlot::FSlotArguments(MakeUnique<FSlot>());
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

FChildren* SArtboardPanel::GetChildren()
{
	return &Children;
}

int32 SArtboardPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                              FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = PaintBackground(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	LayerId = SPanel::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bIsPanning)
	{
		TArray<FVector2D> PanPoints = {
			MouseDownAbsolutePosition,
			MouseDownAbsolutePosition
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			PanPoints,
			ESlateDrawEffect::None,
			FLinearColor::Red
		);
	}

	return LayerId;
}

FReply SArtboardPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SPanel::OnMouseButtonDown(MyGeometry, MouseEvent);

	FReply Reply = FReply::Unhandled();

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = false;
		PanViewOffsetStart = ViewOffset;
		MouseDownAbsolutePosition = MouseEvent.GetLastScreenSpacePosition();

		Reply.CaptureMouse(SharedThis(this));
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || FSlateApplication::Get().IsUsingTrackpad())
	{
		DragDeltaDistance = 0.0f;
		ZoomStartOffset = MyGeometry.AbsoluteToLocal(MouseEvent.GetLastScreenSpacePosition());
		UE_LOG(LogTemp, Log, TEXT("ZoomStartOffset: %s"), *ZoomStartOffset.ToString());

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

	const bool bIsRMBDown = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const bool bIsLMBDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
	const bool bIsMMBDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	const FModifierKeysState ModifierKeysState = FSlateApplication::Get().GetModifierKeys();

	const bool bShouldZoom = bIsRMBDown && (bIsLMBDown || bIsMMBDown || ModifierKeysState.IsAltDown() || FSlateApplication::Get().IsUsingTrackpad());
	if (bShouldZoom)
	{
		const FVector2D CursorDelta = MouseEvent.GetCursorDelta();
		DragDeltaDistance += CursorDelta.X + CursorDelta.Y;

		const float ZoomDelta = DragDeltaDistance * ZoomSensitivity;

		// remove mouse movement that's been 'used up' by zooming
		if (ZoomDelta != 0)
		{
			DragDeltaDistance -= (ZoomDelta / ZoomSensitivity);
			bIsZooming = true;

			ApplyZoomDelta(ZoomDelta, ZoomStartOffset, MyGeometry);

			bIsPanning = false;
		}

		return FReply::Handled();
	}

	if (bIsRMBDown || bIsMMBDown)
	{
		bIsPanning = true;
		bIsZooming = false;

		const FVector2D DeltaViewOffset = (MouseDownAbsolutePosition - MouseEvent.GetScreenSpacePosition()) / MyGeometry.Scale / GetZoomAmount();
		const FVector2D NewViewOffset = PanViewOffsetStart + DeltaViewOffset;
		SetViewOffset(NewViewOffset, MyGeometry);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SArtboardPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// zoom into the focus point; i.e. keep it the same fraction offset in the panel
	const FVector2D FocusPoint = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float ZoomDelta = MouseEvent.GetWheelDelta() * ZoomWheelSensitivity;
	ApplyZoomDelta(ZoomDelta, FocusPoint, MyGeometry);

	return FReply::Handled();
}

int32 SArtboardPanel::PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&BackgroundBrush.Get(),
		ESlateDrawEffect::None,
		BackgroundBrush.Get().TintColor.GetSpecifiedColor()
	);

	// draw artboard frame
	if (bShowArtboardBorder.Get())
	{
		constexpr FLinearColor BorderColor = FLinearColor(1.f, 1.f, 1.f, 0.02f);
		constexpr float ArtboardSize = 512.f;
		const TArray<FVector2D> BorderPoints = {
			GraphCoordToPanelCoord(FVector2D(0, 0) * ArtboardSize) + FVector2D(-2, -2),
			GraphCoordToPanelCoord(FVector2D(1, 0) * ArtboardSize) + FVector2D(2, -2),
			GraphCoordToPanelCoord(FVector2D(1, 1) * ArtboardSize) + FVector2D(2, 2),
			GraphCoordToPanelCoord(FVector2D(0, 1) * ArtboardSize) + FVector2D(-2, 2),
			GraphCoordToPanelCoord(FVector2D(0, 0)) + FVector2D(-2, -2),
		};
		constexpr float BorderThickness = 1.f;

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

void SArtboardPanel::SetViewOffset(FVector2D NewViewOffset, const FGeometry& AlottedGeometry)
{
	// min is the furthest to the bottom-right allowed, which accounts for the widget geometry and current zoom
	const FVector2D ViewOffsetMin = (-AlottedGeometry.GetLocalSize() + ViewOffsetMargin) / GetZoomAmount();
	// max is based only on artboard size, since 0,0 coordinates are top-left
	const FVector2D ViewOffsetMax = ArtboardBounds.GetSize() - (ViewOffsetMargin / GetZoomAmount());
	const FVector2D ClampedViewOffset = FVector2D(
		FMath::Clamp(NewViewOffset.X, ViewOffsetMin.X, ViewOffsetMax.X),
		FMath::Clamp(NewViewOffset.Y, ViewOffsetMin.Y, ViewOffsetMax.Y));

	if (ViewOffset != ClampedViewOffset)
	{
		ViewOffset = ClampedViewOffset;

		UE_LOG(LogTemp, Log, TEXT("ViewOffset: %s"), *ViewOffset.ToString());

		OnViewOffsetChanged();
	}
}

void SArtboardPanel::SetZoomAmount(float NewZoomAmount)
{
	const float ClampedZoomAmount = FMath::Clamp(NewZoomAmount, ZoomAmountMin, ZoomAmountMax);
	if (ZoomAmount != ClampedZoomAmount)
	{
		ZoomAmount = ClampedZoomAmount;

		UE_LOG(LogTemp, Log, TEXT("ZoomAmount: %0.3f"), ZoomAmount);

		OnZoomChanged();
	}
}

void SArtboardPanel::ApplyZoomDelta(float ZoomDelta, const FVector2D& WidgetSpaceZoomOrigin, const FGeometry& AlottedGeometry)
{
	// we want to zoom into this point; i.e. keep it the same fraction offset into the panel
	const FVector2D FocusPoint = PanelCoordToGraphCoord(WidgetSpaceZoomOrigin);

	// instead of just adding, apply proportional so 'zoom speed' is the same at all zoom levels
	SetZoomAmount(ZoomAmount *= (1.f + ZoomDelta));

	// adjust offset to zoom around the target position
	const FVector2D NewViewOffset = FocusPoint - WidgetSpaceZoomOrigin / GetZoomAmount();

	// offset the panning start in case panning occurs later
	PanViewOffsetStart += NewViewOffset - ViewOffset;

	SetViewOffset(NewViewOffset, AlottedGeometry);

	DragDeltaDistance = 0.0f;
}

FVector2D SArtboardPanel::GraphCoordToPanelCoord(const FVector2D& GraphPosition) const
{
	return (GraphPosition - GetViewOffset()) * GetZoomAmount();
}

FVector2D SArtboardPanel::PanelCoordToGraphCoord(const FVector2D& PanelPosition) const
{
	return PanelPosition / GetZoomAmount() + GetViewOffset();
}

void SArtboardPanel::OnViewOffsetChanged()
{
}

void SArtboardPanel::OnZoomChanged()
{
}

FText SArtboardPanel::GetDebugText() const
{
	return FText::FromString(TEXT("Hello"));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
