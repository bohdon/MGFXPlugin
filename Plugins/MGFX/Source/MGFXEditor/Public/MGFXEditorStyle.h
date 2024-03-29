﻿// Copyright Bohdon Sayre, All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"


/**
 * Custom editor slate style for the MGFX plugin.
 */
class FMGFXEditorStyle final : public FSlateStyleSet
{
public:
	FMGFXEditorStyle();

	~FMGFXEditorStyle();

public:
	static const FName StaticStyleSetName;

	static FMGFXEditorStyle& Get();
};
