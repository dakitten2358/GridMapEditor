#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"

class FGridMapEditorMode;

class SGridMapEditorToolkitWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridMapEditorToolkitWidget) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

private:	// construction helpers
	TSharedRef<SWidget> BuildToolBar();

private:
	bool IsGridMapEditorEnabled() const;
	bool IsPaintTool() const;

	FText GetActiveToolName() const;
	FText GetGridMapEditorErrorText() const;

	// helpers for getting/setting ui
	void SetPaintHeight(float InHeight);
	TOptional<float> GetPaintHeight() const;

	void OnChangeTileSet(UObject* NewAsset);
	UObject* GetCurrentTileSet() const;

	void OnCheckStateChanged_HideOwnedActors(ECheckBoxState InState);
	ECheckBoxState GetCheckState_HideOwnedActors() const;
	
	// helper for visibilities
	EVisibility GetVisibility_PaintOptions() const;

private:

	/** Current error message */
	TSharedPtr<class SErrorText> ErrorText;
	FGridMapEditorMode* GridMapEditorMode;
};