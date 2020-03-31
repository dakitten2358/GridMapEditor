#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

struct FGridMapEditorUISettings;

class SGridMapEditorSettingsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridMapEditorSettingsWidget) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, FGridMapEditorUISettings* GridMapUISettings);

private:
	EVisibility GetVisibility_SettingsTab() const;

	void OnCheckStateChanged_DrawUpdatedTiles(ECheckBoxState InState);
	ECheckBoxState GetCheckState_DrawUpdatedTiles() const;

private:
	FGridMapEditorUISettings* UISettings;
};
