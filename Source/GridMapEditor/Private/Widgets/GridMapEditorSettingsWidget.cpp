#include "Widgets/GridMapEditorSettingsWidget.h"
#include "GridMapEditorUISettings.h"
#include "GridMapStyleSet.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GridMapEditor"

void SGridMapEditorSettingsWidget::Construct(const FArguments& InArgs, FGridMapEditorUISettings* GridMapUISettings)
{
	UISettings = GridMapUISettings;

	ChildSlot[
		SNew(SVerticalBox)
		.Visibility(this, &SGridMapEditorSettingsWidget::GetVisibility_SettingsTab)
		// Debug Options
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FGridMapStyleSet::StandardPadding)
		[
			SNew(SHeader)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DebugOptionHeader", "Debug Options"))
				.Font(FGridMapStyleSet::StandardFont)
			]
		]
		// Debug Options - Draw Updated Tiles
		// Hide Actors
		+ SVerticalBox::Slot()
		.Padding(FGridMapStyleSet::StandardPadding)
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(FGridMapStyleSet::StandardPadding)
			[
				SNew(SWrapBox)
				.UseAllottedWidth(true)
				.InnerSlotPadding({6, 5})

				+ SWrapBox::Slot()
				[
					SNew(SBox)
					.MinDesiredWidth(150)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SGridMapEditorSettingsWidget::OnCheckStateChanged_DrawUpdatedTiles)
						.IsChecked(this, &SGridMapEditorSettingsWidget::GetCheckState_DrawUpdatedTiles)
						.ToolTipText(LOCTEXT("DebugOption_DrawUpdatedTiles", "Whether to draw debug info for updated tiles"))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DebugOption_DrawUpdatedTiles_Label", "Show Updated Tiles"))
							.Font(FGridMapStyleSet::StandardFont)
						]
					]
				]
			]
		]
	];
}

EVisibility SGridMapEditorSettingsWidget::GetVisibility_SettingsTab() const
{
	if (UISettings && UISettings->GetSettingsToolSelected())
		return EVisibility::Visible;

	return EVisibility::Collapsed;
}

void SGridMapEditorSettingsWidget::OnCheckStateChanged_DrawUpdatedTiles(ECheckBoxState InState)
{
	if (UISettings)
	{
		UISettings->SetDebugDrawTiles(InState == ECheckBoxState::Checked ? true : false);
	}
}

ECheckBoxState SGridMapEditorSettingsWidget::GetCheckState_DrawUpdatedTiles() const
{
	if (UISettings && UISettings->GetDebugDrawTiles())
		return ECheckBoxState::Checked;

	return ECheckBoxState::Unchecked;
}


#undef LOCTEXT_NAMESPACE