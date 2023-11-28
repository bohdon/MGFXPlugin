// Copyright Bohdon Sayre, All Rights Reserved.


#include "SMGFXMaterialEditorCanvas.h"

#include "EditorViewportCommands.h"
#include "MGFXMaterial.h"
#include "MGFXMaterialEditor.h"
#include "MGFXMaterialEditorCanvasToolBar.h"
#include "MGFXMaterialEditorCommands.h"
#include "MGFXPropertyMacros.h"
#include "SArtboardPanel.h"
#include "SlateOptMacros.h"
#include "SMGFXShapeTransformHandle.h"
#include "Styling/ToolBarStyle.h"
#include "Widgets/SCanvas.h"


#define LOCTEXT_NAMESPACE "MGFXMaterialEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGFXMaterialEditorCanvas::SMGFXMaterialEditorCanvas()
	: TransformMode(EMGFXShapeTransformMode::Select),
	  bAlwaysShowArtboardBorder(true)
{
}

void SMGFXMaterialEditorCanvas::Construct(const FArguments& InArgs)
{
	check(InArgs._MGFXMaterialEditor.IsValid());

	MGFXMaterialEditor = InArgs._MGFXMaterialEditor;

	MGFXMaterialEditor.Pin()->OnLayerSelectionChangedEvent.AddSP(this, &SMGFXMaterialEditorCanvas::OnLayerSelectionChanged);
	MGFXMaterialEditor.Pin()->OnPreviewMaterialChangedEvent.AddSP(this, &SMGFXMaterialEditorCanvas::OnPreviewMaterialChanged);

	if (UMaterialInstanceDynamic* PreviewMaterial = MGFXMaterialEditor.Pin()->GetPreviewMaterial())
	{
		PreviewImageBrush.SetResourceObject(PreviewMaterial);
	}

	// TODO: expose as view option
	bAlwaysShowArtboardBorder = true;

	SelectionOutlineAnim = FCurveSequence(0.0f, 0.15f, ECurveEaseFunction::QuadOut);

	CommandList = MakeShareable(new FUICommandList);
	FMGFXMaterialEditorCommands::Register();
	FEditorViewportCommands::Register();
	BindCommands();

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(ArtboardPanel, SArtboardPanel)
			.ArtboardSize(this, &SMGFXMaterialEditorCanvas::GetArtboardSize)
			.BackgroundBrush(this, &SMGFXMaterialEditorCanvas::GetArtboardBackground)
			.bShowArtboardBorder(this, &SMGFXMaterialEditorCanvas::ShouldShowArtboardBorder)
			.ZoomAmountMax(100.f)
			.OnViewOffsetChanged(this, &SMGFXMaterialEditorCanvas::OnViewOffsetChanged)
			.OnZoomChanged(this, &SMGFXMaterialEditorCanvas::OnZoomChanged)
			.Clipping(EWidgetClipping::ClipToBounds)

			// add preview material image, full size of the artboard
			+ SArtboardPanel::Slot()
			  .Position(FVector2D::ZeroVector)
			  .Size(GetArtboardSize())
			[
				SAssignNew(PreviewImage, SImage)
				.Image(&PreviewImageBrush)
			]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(EditingWidgetCanvas, SCanvas)
		]

		+ SOverlay::Slot()
		[
			CreateOverlayUI()
		]
	];
}

TSharedRef<SWidget> SMGFXMaterialEditorCanvas::CreateOverlayUI()
{
	const FToolBarStyle& ToolBarStyle = FAppStyle::Get().GetWidgetStyle<FToolBarStyle>("EditorViewportToolBar");

	return SNew(SOverlay)

		// top bar with buttons for view and editing options
		+ SOverlay::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Top)
		[
			SNew(SHorizontalBox)

			// fill spacer
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
				.Size(FVector2D(1, 1))
			]

			// toolbar for transform modes, etc
			+ SHorizontalBox::Slot()
			  .Padding(0.0f, 2.0f)
			  .VAlign(VAlign_Center)
			  .AutoWidth()
			[
				SNew(SMGFXMaterialEditorCanvasToolBar)
				.CommandList(CommandList)
			]

			// zoom menu
			+ SHorizontalBox::Slot()
			  .Padding(2.0f, 2.0f)
			  .AutoWidth()
			[
				SNew(SComboButton)
				.ButtonStyle(&ToolBarStyle.ButtonStyle)
				.OnGetMenuContent(this, &SMGFXMaterialEditorCanvas::CreateZoomPresetsMenu)
				.ContentPadding(ToolBarStyle.ComboButtonPadding)
				.ToolTipText(LOCTEXT("CurrentZoom", "Current Zoom"))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SMGFXMaterialEditorCanvas::GetZoomPresetsMenULabel)
					.TextStyle(&ToolBarStyle.LabelStyle)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];
}

FVector2D SMGFXMaterialEditorCanvas::GetArtboardSize() const
{
	return MGFXMaterialEditor.Pin()->GetCanvasSize();
}

const FSlateBrush* SMGFXMaterialEditorCanvas::GetArtboardBackground() const
{
	const UMGFXMaterial* MGFXMaterial = MGFXMaterialEditor.Pin()->GetMGFXMaterial();
	return MGFXMaterial->bOverrideDesignerBackground ? &MGFXMaterial->DesignerBackground : nullptr;
}

bool SMGFXMaterialEditorCanvas::ShouldShowArtboardBorder() const
{
	if (bAlwaysShowArtboardBorder)
	{
		return true;
	}

	// when the preview image doesn't match the current artboard settings, force display the border
	const SArtboardPanel::FSlot* Slot = ArtboardPanel->GetWidgetSlot(PreviewImage);
	return Slot->GetSize() != GetArtboardSize();
}

void SMGFXMaterialEditorCanvas::UpdateArtboardSize()
{
	if (const UMGFXMaterial* Material = GetMGFXMaterial())
	{
		SArtboardPanel::FSlot* Slot = ArtboardPanel->GetWidgetSlot(PreviewImage);
		check(Slot);
		Slot->SetSize(Material->BaseCanvasSize);
	}
}

void SMGFXMaterialEditorCanvas::OnPreviewMaterialChanged(UMaterialInterface* NewMaterial)
{
	PreviewImageBrush.SetResourceObject(NewMaterial);
}

UMGFXMaterial* SMGFXMaterialEditorCanvas::GetMGFXMaterial() const
{
	return MGFXMaterialEditor.Pin()->GetMGFXMaterial();
}

FReply SMGFXMaterialEditorCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// middle mouse to free transform the selected layer.
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && !MouseEvent.IsAltDown())
	{
		if (GetSelectedLayer() && TransformHandle.IsValid())
		{
			// this will either be a TranslateXY, Rotate, or ScaleXY operation
			return TransformHandle->StartDragging(MouseEvent);
		}

		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SMGFXMaterialEditorCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

int32 SMGFXMaterialEditorCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                         FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	++LayerId;
	LayerId = PaintSelectionOutline(ArtboardPanel->GetPaintSpaceGeometry(), MyCullingRect, OutDrawElements, LayerId);

	return LayerId;
}

int32 SMGFXMaterialEditorCanvas::PaintSelectionOutline(const FGeometry& AllottedGeometry, FSlateRect MyCullingRect,
                                                       FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	TArray<UMGFXMaterialLayer*> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();

	const FLinearColor OutlineColor = FStyleColors::Select.GetSpecifiedColor();

	// flash a broad width selection outline to draw attention
	const float OutlineWidth = FMath::Lerp(3.f, 1.f, SelectionOutlineAnim.GetLerp());

	for (const UMGFXMaterialLayer* SelectedLayer : SelectedLayers)
	{
		if (!SelectedLayer->HasBounds())
		{
			// no bounds to represent
			continue;
		}

		// get layer transform including artboard view offset and scale
		const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
		const FTransform2D LayerTransform = SelectedLayer->GetTransform().Concatenate(ArtboardTransform);

		const FBox2D LayerBounds = SelectedLayer->GetBounds();

		FGeometry LayerGeometry = AllottedGeometry
		                          .MakeChild(LayerTransform, FVector2f::ZeroVector)
		                          .MakeChild(LayerBounds.GetSize(), FSlateLayoutTransform(LayerBounds.Min));

		// convert to paint geometry, and inflate by outline size
		const FVector2D LayerScaleVector = FVector2D(LayerGeometry.GetAccumulatedRenderTransform().GetMatrix().GetScale().GetVector());
		const FVector2D OutlinePixelSize = FVector2D(OutlineWidth, OutlineWidth) / LayerScaleVector;
		FPaintGeometry SelectionGeometry = LayerGeometry.ToInflatedPaintGeometry(OutlinePixelSize);

		FSlateClippingZone SelectionZone(SelectionGeometry);
		const TArray<FVector2D> Points = {
			FVector2D(SelectionZone.TopLeft),
			FVector2D(SelectionZone.TopRight),
			FVector2D(SelectionZone.BottomRight),
			FVector2D(SelectionZone.BottomLeft),
			FVector2D(SelectionZone.TopLeft),
		};

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			FPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			OutlineColor,
			true,
			OutlineWidth);
	}

	return LayerId;
}

void SMGFXMaterialEditorCanvas::BindCommands()
{
	// transform mode actions
	CommandList->MapAction(
		FEditorViewportCommands::Get().SelectMode,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetTransformMode, EMGFXShapeTransformMode::Select),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::CanSetTransformMode, EMGFXShapeTransformMode::Select),
		FIsActionChecked::CreateSP(this, &SMGFXMaterialEditorCanvas::IsTransformModeActive, EMGFXShapeTransformMode::Select));

	CommandList->MapAction(
		FEditorViewportCommands::Get().TranslateMode,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetTransformMode, EMGFXShapeTransformMode::Translate),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::CanSetTransformMode, EMGFXShapeTransformMode::Translate),
		FIsActionChecked::CreateSP(this, &SMGFXMaterialEditorCanvas::IsTransformModeActive, EMGFXShapeTransformMode::Translate));

	CommandList->MapAction(
		FEditorViewportCommands::Get().RotateMode,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetTransformMode, EMGFXShapeTransformMode::Rotate),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::CanSetTransformMode, EMGFXShapeTransformMode::Rotate),
		FIsActionChecked::CreateSP(this, &SMGFXMaterialEditorCanvas::IsTransformModeActive, EMGFXShapeTransformMode::Rotate));

	CommandList->MapAction(
		FEditorViewportCommands::Get().ScaleMode,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetTransformMode, EMGFXShapeTransformMode::Scale),
		FCanExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::CanSetTransformMode, EMGFXShapeTransformMode::Scale),
		FIsActionChecked::CreateSP(this, &SMGFXMaterialEditorCanvas::IsTransformModeActive, EMGFXShapeTransformMode::Scale));

	// view actions
	CommandList->MapAction(
		FMGFXMaterialEditorCommands::Get().ZoomTo50,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetZoomAmount, 0.5f));

	CommandList->MapAction(
		FMGFXMaterialEditorCommands::Get().ZoomTo100,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetZoomAmount, 1.f));

	CommandList->MapAction(
		FMGFXMaterialEditorCommands::Get().ZoomTo200,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetZoomAmount, 2.f));

	CommandList->MapAction(
		FMGFXMaterialEditorCommands::Get().ZoomTo300,
		FExecuteAction::CreateSP(this, &SMGFXMaterialEditorCanvas::SetZoomAmount, 3.f));
}

void SMGFXMaterialEditorCanvas::OnViewOffsetChanged(FVector2D NewViewOffset)
{
}

void SMGFXMaterialEditorCanvas::OnZoomChanged(float NewZoomAmount)
{
}

void SMGFXMaterialEditorCanvas::OnLayerSelectionChanged(const TArray<TObjectPtr<UMGFXMaterialLayer>>& SelectedLayers)
{
	SelectionOutlineAnim.Play(SharedThis(this));

	// clear and recreate widgets to interactively transform or modify the layer
	CreateEditingWidgets();
}

void SMGFXMaterialEditorCanvas::CreateEditingWidgets()
{
	// clear existing widgets
	TransformHandle.Reset();
	EditingWidgetCanvas->ClearChildren();

	if (!GetSelectedLayer())
	{
		return;
	}

	// create transform handle
	SAssignNew(TransformHandle, SMGFXShapeTransformHandle)
		.Mode(this, &SMGFXMaterialEditorCanvas::GetTransformMode)
		.ParentTransform(this, &SMGFXMaterialEditorCanvas::GetTransformHandleParentTransform)
		.Location(this, &SMGFXMaterialEditorCanvas::GetTransformHandleLocation)
		.Rotation(this, &SMGFXMaterialEditorCanvas::GetTransformHandleRotation)
		.Scale(this, &SMGFXMaterialEditorCanvas::GetTransformHandleScale)
		.OnSetLocation(this, &SMGFXMaterialEditorCanvas::OnSetLayerLocation)
		.OnSetRotation(this, &SMGFXMaterialEditorCanvas::OnSetLayerRotation)
		.OnSetScale(this, &SMGFXMaterialEditorCanvas::OnSetLayerScale);

	const TSharedRef<SMGFXShapeTransformHandle> TransformHandleRef = TransformHandle.ToSharedRef();
	EditingWidgetCanvas
		->AddSlot()
		.Position(this, &SMGFXMaterialEditorCanvas::GetEditingWidgetPosition, StaticCastSharedRef<SWidget>(TransformHandleRef))
		.Size(this, &SMGFXMaterialEditorCanvas::GetEditingWidgetSize, StaticCastSharedRef<SWidget>(TransformHandleRef))
		[
			TransformHandleRef
		];
}

FVector2D SMGFXMaterialEditorCanvas::GetEditingWidgetPosition(TSharedRef<SWidget> EditingWidget) const
{
	const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer();
	if (!SelectedLayer)
	{
		return FVector2D::ZeroVector;
	}

	// position will be relative to this canvas's geometry, so just perform the canvas -> layer position transformation
	const FTransform2D LayerTransform = SelectedLayer->GetTransform();
	const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
	const FVector2D Position = LayerTransform.Concatenate(ArtboardTransform).TransformPoint(FVector2D::ZeroVector);

	return Position;
}

FVector2D SMGFXMaterialEditorCanvas::GetEditingWidgetSize(TSharedRef<SWidget> EditingWidget) const
{
	return EditingWidget->GetDesiredSize();
}

FTransform2D SMGFXMaterialEditorCanvas::GetTransformHandleParentTransform() const
{
	if (const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		const FTransform2D ParentTransform = SelectedLayer->GetParentTransform();
		const FTransform2D ArtboardTransform = ArtboardPanel->GetPanelToGraphTransform();
		return ParentTransform.Concatenate(ArtboardTransform);
	}
	return FTransform2D();
}

FVector2D SMGFXMaterialEditorCanvas::GetTransformHandleLocation() const
{
	if (const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		return FVector2D(SelectedLayer->Transform.Location);
	}
	return FVector2D::ZeroVector;
}

float SMGFXMaterialEditorCanvas::GetTransformHandleRotation() const
{
	if (const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		return SelectedLayer->Transform.Rotation;
	}
	return 0.f;
}

FVector2D SMGFXMaterialEditorCanvas::GetTransformHandleScale() const
{
	if (const UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		return FVector2D(SelectedLayer->Transform.Scale);
	}
	return FVector2D::ZeroVector;
}

void SMGFXMaterialEditorCanvas::OnSetLayerLocation(FVector2D NewLocation, bool bIsFinished)
{
	if (UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		SelectedLayer->Modify();
		SelectedLayer->Transform.Location = FVector2f(NewLocation);

		// send property change event
		const EPropertyChangeType::Type ChangeType = bIsFinished ? EPropertyChangeType::ValueSet : EPropertyChangeType::Interactive;
		FProperty* LocationProp = PropertyAccessUtil::FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Location), FMGFXShapeTransform2D::StaticStruct());
		SendLayerTransformPropertyChangeEvent(SelectedLayer, ChangeType, LocationProp);
	}
}

void SMGFXMaterialEditorCanvas::OnSetLayerRotation(float NewRotation, bool bIsFinished)
{
	if (UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		SelectedLayer->Modify();
		SelectedLayer->Transform.Rotation = NewRotation;

		// send property change event
		const EPropertyChangeType::Type ChangeType = bIsFinished ? EPropertyChangeType::ValueSet : EPropertyChangeType::Interactive;
		FProperty* RotationProp = PropertyAccessUtil::FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Rotation), FMGFXShapeTransform2D::StaticStruct());
		SendLayerTransformPropertyChangeEvent(SelectedLayer, ChangeType, RotationProp);
	}
}

void SMGFXMaterialEditorCanvas::OnSetLayerScale(FVector2D NewScale, bool bIsFinished)
{
	if (UMGFXMaterialLayer* SelectedLayer = GetSelectedLayer())
	{
		SelectedLayer->Modify();
		SelectedLayer->Transform.Scale = FVector2f(NewScale);

		// send property change event
		const EPropertyChangeType::Type ChangeType = bIsFinished ? EPropertyChangeType::ValueSet : EPropertyChangeType::Interactive;
		FProperty* ScaleProp = PropertyAccessUtil::FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(FMGFXShapeTransform2D, Scale), FMGFXShapeTransform2D::StaticStruct());
		SendLayerTransformPropertyChangeEvent(SelectedLayer, ChangeType, ScaleProp);
	}
}

void SMGFXMaterialEditorCanvas::SendLayerTransformPropertyChangeEvent(UMGFXMaterialLayer* Layer, EPropertyChangeType::Type ChangeType, FProperty* Property)
{
	// send property change event
	FProperty* TransformProp = PropertyAccessUtil::FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(UMGFXMaterialLayer, Transform), UMGFXMaterialLayer::StaticClass());

	FEditPropertyChain PropertyChain;
	PropertyChain.AddHead(TransformProp);
	PropertyChain.AddTail(Property);
	PropertyChain.SetActivePropertyNode(Property);
	PropertyChain.SetActiveMemberPropertyNode(TransformProp);

	FPropertyChangedEvent PropertyChangedEvent(Property, ChangeType, {Layer});
	PropertyChangedEvent.SetActiveMemberProperty(TransformProp);

	Layer->PostEditChangeProperty(PropertyChangedEvent);
	MGFXMaterialEditor.Pin()->NotifyPostChange(PropertyChangedEvent, &PropertyChain);
}

UMGFXMaterialLayer* SMGFXMaterialEditorCanvas::GetSelectedLayer() const
{
	TArray<TObjectPtr<UMGFXMaterialLayer>> SelectedLayers = MGFXMaterialEditor.Pin()->GetSelectedLayers();
	if (SelectedLayers.Num() == 1)
	{
		return SelectedLayers[0];
	}
	return nullptr;
}

void SMGFXMaterialEditorCanvas::SetTransformMode(EMGFXShapeTransformMode NewMode)
{
	TransformMode = NewMode;
}

bool SMGFXMaterialEditorCanvas::CanSetTransformMode(EMGFXShapeTransformMode NewMode)
{
	return true;
}

bool SMGFXMaterialEditorCanvas::IsTransformModeActive(EMGFXShapeTransformMode InMode)
{
	return InMode == TransformMode;
}

FText SMGFXMaterialEditorCanvas::GetZoomPresetsMenULabel() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;

	return FText::AsPercent(ArtboardPanel->GetZoomAmount(), &Options);
}

TSharedRef<SWidget> SMGFXMaterialEditorCanvas::CreateZoomPresetsMenu()
{
	FMenuBuilder MenuBuilder(true, CommandList);

	MenuBuilder.BeginSection(TEXT("ZoomPresets"));
	MenuBuilder.AddMenuEntry(FMGFXMaterialEditorCommands::Get().ZoomTo50);
	MenuBuilder.AddMenuEntry(FMGFXMaterialEditorCommands::Get().ZoomTo100);
	MenuBuilder.AddMenuEntry(FMGFXMaterialEditorCommands::Get().ZoomTo200);
	MenuBuilder.AddMenuEntry(FMGFXMaterialEditorCommands::Get().ZoomTo300);
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SMGFXMaterialEditorCanvas::SetZoomAmount(float NewZoomAmount)
{
	ArtboardPanel->SetZoomAmountPreserveCenter(NewZoomAmount);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
