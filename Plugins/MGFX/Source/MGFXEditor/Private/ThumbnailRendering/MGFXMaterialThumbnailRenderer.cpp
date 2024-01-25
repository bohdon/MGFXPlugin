// Copyright Bohdon Sayre, All Rights Reserved.


#include "ThumbnailRendering/MGFXMaterialThumbnailRenderer.h"

#include "MGFXMaterial.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "SlateMaterialBrush.h"
#include "ThumbnailHelpers.h"
#include "Engine/Texture2D.h"
#include "Slate/WidgetRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Widgets/Images/SImage.h"


void UMGFXMaterialThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas,
                                          bool bAdditionalViewFamily)
{
	// copied from (not-public) UMaterialInstanceThumbnailRenderer

	const UMGFXMaterial* MGFXMaterial = Cast<UMGFXMaterial>(Object);

	if (!MGFXMaterial->Material)
	{
		return;
	}

	UMaterialInterface* MatInst = MGFXMaterial->Material;
	if (MatInst != nullptr)
	{
		UMaterial* Mat = MatInst->GetMaterial();
		if (Mat != nullptr && Mat->IsUIMaterial())
		{
			if (WidgetRenderer == nullptr)
			{
				const bool bUseGammaCorrection = true;
				WidgetRenderer = new FWidgetRenderer(bUseGammaCorrection);
				check(WidgetRenderer);
			}

			if (UIMaterialBrush == nullptr)
			{
				UIMaterialBrush = new FSlateMaterialBrush(FVector2D(SlateBrushDefs::DefaultImageSize, SlateBrushDefs::DefaultImageSize));
				check(UIMaterialBrush);
			}

			UIMaterialBrush->SetMaterial(MatInst);

			UTexture2D* CheckerboardTexture = UThumbnailManager::Get().CheckerboardTexture;

			FSlateBrush CheckerboardBrush;
			CheckerboardBrush.SetResourceObject(CheckerboardTexture);
			CheckerboardBrush.ImageSize = FVector2D(CheckerboardTexture->GetSizeX(), CheckerboardTexture->GetSizeY());
			CheckerboardBrush.Tiling = ESlateBrushTileType::Both;

			TSharedRef<SWidget> Thumbnail =
				SNew(SOverlay)

				// Checkerboard
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(&CheckerboardBrush)
				]

				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(UIMaterialBrush)
				];

			const FVector2D DrawSize((float)Width, (float)Height);
			const float DeltaTime = 0.f;
			WidgetRenderer->DrawWidget(RenderTarget, Thumbnail, DrawSize, DeltaTime);

			UIMaterialBrush->SetMaterial(nullptr);
		}
		else
		{
			if (ThumbnailScene == nullptr || ensure(ThumbnailScene->GetWorld() != nullptr) == false)
			{
				if (ThumbnailScene)
				{
					FlushRenderingCommands();
					delete ThumbnailScene;
				}
				ThumbnailScene = new FMaterialThumbnailScene();
			}

			ThumbnailScene->SetMaterialInterface(MatInst);
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game))
			                                   .SetTime(UThumbnailRenderer::GetTime())
			                                   .SetAdditionalViewFamily(bAdditionalViewFamily));

			ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
			ViewFamily.EngineShowFlags.SetSeparateTranslucency(ThumbnailScene->ShouldSetSeparateTranslucency(MatInst));
			ViewFamily.EngineShowFlags.MotionBlur = 0;
			ViewFamily.EngineShowFlags.AntiAliasing = 0;

			RenderViewFamily(Canvas, &ViewFamily, ThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));

			ThumbnailScene->SetMaterialInterface(nullptr);
		}
	}
}

void UMGFXMaterialThumbnailRenderer::BeginDestroy()
{
	if (ThumbnailScene != nullptr)
	{
		delete ThumbnailScene;
		ThumbnailScene = nullptr;
	}

	if (WidgetRenderer != nullptr)
	{
		BeginCleanup(WidgetRenderer);
		WidgetRenderer = nullptr;
	}

	if (UIMaterialBrush != nullptr)
	{
		delete UIMaterialBrush;
		UIMaterialBrush = nullptr;
	}

	Super::BeginDestroy();
}
