// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "MGFXMaterialThumbnailRenderer.generated.h"

class FMaterialThumbnailScene;
class FWidgetRenderer;
struct FSlateMaterialBrush;


/**
 * Thumbnail renderer for a MGFXMaterial
 */
UCLASS()
class MGFXEDITOR_API UMGFXMaterialThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas,
	                  bool bAdditionalViewFamily) override;

	virtual void BeginDestroy() override;

private:
	FMaterialThumbnailScene* ThumbnailScene;
	FWidgetRenderer* WidgetRenderer;
	FSlateMaterialBrush* UIMaterialBrush;
};
