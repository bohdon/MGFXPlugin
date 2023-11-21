// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SViewportToolBar.h"
#include "Widgets/SCompoundWidget.h"


/**
 * Toolbar displayed in the canvas panel of the MGFX Material Editor
 */
class MGFXEDITOR_API SMGFXMaterialEditorCanvasToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(SMGFXMaterialEditorCanvasToolBar)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, CommandList)
		SLATE_ARGUMENT(TSharedPtr<FExtender>, Extenders)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
