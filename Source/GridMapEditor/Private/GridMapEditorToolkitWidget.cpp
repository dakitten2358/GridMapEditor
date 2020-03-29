#include "GridMapEditorToolkitWidget.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GridMapEditCommands.h"
#include "GridMapEditorMode.h"
#include "SlateOptMacros.h"
#include "TileSet.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SErrorText.h"
#include "WorkflowOrientedApp/SContentReference.h"

#define LOCTEXT_NAMESPACE "GridMapEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGridMapEditorToolkitWidget::Construct(const FArguments& InArgs)
{
	GridMapEditorMode = (FGridMapEditorMode*)GLevelEditorModeTools().GetActiveMode(FGridMapEditorMode::EM_GridMapEditorModeId);

	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(6.f, 3.f);
	FMargin StandardLeftPadding(6.f, 3.f, 3.f, 3.f);
	FMargin StandardRightPadding(3.f, 3.f, 6.f, 3.f);

	FSlateFontInfo StandardFont = FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

	const FText BlankText = FText::GetEmpty();

	ChildSlot[
		SNew(SVerticalBox)
		// Error text goes in the first vertical slot
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,0,0,5)
		[
			SAssignNew(ErrorText, SErrorText)
		]
		// Main content comes next
		+ SVerticalBox::Slot()
		.Padding(0)
		[
			// Container for left side "tabs" and right content
			SNew(SVerticalBox)
			.IsEnabled(this, &SGridMapEditorToolkitWidget::IsGridMapEditorEnabled)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				// left side "tabs"
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1.f, 5.f, 0.f, 5.f)
				[
					BuildToolBar()
				]
				// right side content
				+ SHorizontalBox::Slot()
				.Padding(0.f, 2.f, 2.f, 0.f)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(StandardPadding)
					[
						SNew(SVerticalBox)

						// Active Tool Title
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.Padding(StandardLeftPadding)
							.HAlign(HAlign_Left)
							[
								SNew(STextBlock)
								.Text(this, &SGridMapEditorToolkitWidget::GetActiveToolName)
								.TextStyle(FEditorStyle::Get(), "FoliageEditMode.ActiveToolName.Text")
							]
						]

						// Brush Options
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(StandardPadding)
						[
							SNew(SHeader)
							.Visibility(this, &SGridMapEditorToolkitWidget::GetVisibility_PaintOptions)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("OptionHeader", "Tile Options"))
								.Font(StandardFont)
							]
						]

						//
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							.ToolTipText(LOCTEXT("GridMapActiveTileset_ToolTip", "Currently Active TileSet"))
							.Visibility(this, &SGridMapEditorToolkitWidget::GetVisibility_PaintOptions)

							+ SHorizontalBox::Slot()
							.Padding(StandardLeftPadding)
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("GridMapActiveTileset", "Tiles"))
								.Font(StandardFont)
							]

							+ SHorizontalBox::Slot()
							.Padding(StandardRightPadding)
							.FillWidth(2.0f)
							.MaxWidth(140.f)
							.VAlign(VAlign_Center)
							[
								SNew(SContentReference)
								.WidthOverride(140.f)
								.AssetReference(this, &SGridMapEditorToolkitWidget::GetCurrentTileSet)
								.OnSetReference(this, &SGridMapEditorToolkitWidget::OnChangeTileSet)
								.AllowedClass(UGridMapTileSet::StaticClass())
								.AllowSelectingNewAsset(true)
								.AllowClearingReference(false)
							]								
						]

						// Tilemap Target Height
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							.ToolTipText(LOCTEXT("GridMapPaintHeight_ToolTip", "Paint height of the gridmap"))
							.Visibility(this, &SGridMapEditorToolkitWidget::GetVisibility_PaintOptions)

							+ SHorizontalBox::Slot()
							.Padding(StandardLeftPadding)
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("GridMapPaintHeight", "Height"))
								.Font(StandardFont)
							]
							+ SHorizontalBox::Slot()
							.Padding(StandardRightPadding)
							.FillWidth(2.0f)
							.MaxWidth(100.f)
							.VAlign(VAlign_Center)
							[
								SNew(SNumericEntryBox<float>)
								.Font(StandardFont)
								.AllowSpin(true)
								.MinValue(-65530.0f)
								.MaxValue(65530.0f)
								.MaxSliderValue(1000.f)
								.MinDesiredValueWidth(50.0f)
								.SliderExponent(3.0f)
								.Value(this, &SGridMapEditorToolkitWidget::GetPaintHeight)
								.OnValueChanged(this, &SGridMapEditorToolkitWidget::SetPaintHeight)
							]
						]

						// Hide Actors
						+ SVerticalBox::Slot()
						.Padding(StandardPadding)
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							.Visibility(this, &SGridMapEditorToolkitWidget::GetVisibility_PaintOptions)

							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.Padding(StandardPadding)
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
										.OnCheckStateChanged(this, &SGridMapEditorToolkitWidget::OnCheckStateChanged_HideOwnedActors)
										.IsChecked(this, &SGridMapEditorToolkitWidget::GetCheckState_HideOwnedActors)
										.ToolTipText(LOCTEXT("GridMapHideActors_ToolTip", "Whether to hide the actors in the outliner"))
										[
											SNew(STextBlock)
											.Text(LOCTEXT("GridMapHideActors", "Hide in outliner"))
											.Font(StandardFont)
										]
									]
								]
							]
						]
						
						//

					]
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SGridMapEditorToolkitWidget::BuildToolBar()
{
	FToolBarBuilder Toolbar(GridMapEditorMode->UICommandList, FMultiBoxCustomization::None, nullptr, Orient_Vertical);
	Toolbar.SetLabelVisibility(EVisibility::Collapsed);
	Toolbar.SetStyle(&FEditorStyle::Get(), "FoliageEditToolbar");
	{
		Toolbar.AddToolBarButton(FGridMapEditCommands::Get().SetPaintTiles);
		/*
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetReapplySettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetLassoSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaintBucket);
		*/
	}

	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.HAlign(HAlign_Center)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
				[
					Toolbar.MakeWidget()
				]
			]
		];
}

bool SGridMapEditorToolkitWidget::IsGridMapEditorEnabled() const
{
	ErrorText->SetError(GetGridMapEditorErrorText());
	return GridMapEditorMode->IsEditingEnabled();
}

bool SGridMapEditorToolkitWidget::IsPaintTool() const
{
	return GridMapEditorMode->UISettings.GetPaintToolSelected();
}

void SGridMapEditorToolkitWidget::SetPaintHeight(float InHeight)
{
	GridMapEditorMode->UISettings.SetPaintHeight(InHeight);
}

TOptional<float> SGridMapEditorToolkitWidget::GetPaintHeight() const
{
	return GridMapEditorMode->UISettings.GetPaintHeight();
}

void SGridMapEditorToolkitWidget::OnChangeTileSet(UObject* NewAsset)
{
	if (UGridMapTileSet* NewTileSet = Cast<UGridMapTileSet>(NewAsset))
	{
		GridMapEditorMode->UISettings.SetCurrentTileSet(NewTileSet);
	}
}

UObject* SGridMapEditorToolkitWidget::GetCurrentTileSet() const
{
	return GridMapEditorMode->UISettings.GetCurrentTileSet().Get();
}


void SGridMapEditorToolkitWidget::OnCheckStateChanged_HideOwnedActors(ECheckBoxState InState)
{
	GridMapEditorMode->UISettings.SetHideOwnedActors(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SGridMapEditorToolkitWidget::GetCheckState_HideOwnedActors() const
{
	return GridMapEditorMode->UISettings.GetHideOwnedActors() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}


FText SGridMapEditorToolkitWidget::GetActiveToolName() const
{
	FText OutText;
	if (IsPaintTool())
	{
		OutText = LOCTEXT("FoliageToolName_Paint", "Paint");
	}
	/*
	else if (IsReapplySettingsTool())
	{
		OutText = LOCTEXT("FoliageToolName_Reapply", "Reapply");
	}
	else if (IsSelectTool())
	{
		OutText = LOCTEXT("FoliageToolName_Select", "Select");
	}
	else if (IsLassoSelectTool())
	{
		OutText = LOCTEXT("FoliageToolName_LassoSelect", "Lasso Select");
	}
	else if (IsPaintFillTool())
	{
		OutText = LOCTEXT("FoliageToolName_Fill", "Fill");
	}
	*/

	return OutText;
}

FText SGridMapEditorToolkitWidget::GetGridMapEditorErrorText() const
{
	EGridMapEditingState EditState = GridMapEditorMode->GetEditingState();

	switch (EditState)
	{
	case EGridMapEditingState::SIEWorld: return LOCTEXT("IsSimulatingError_edit", "Can't edit grid map while simulating!");
	case EGridMapEditingState::PIEWorld: return LOCTEXT("IsPIEError_edit", "Can't edit grid map in PIE!");
	case EGridMapEditingState::Enabled: return FText::GetEmpty();
	default: checkNoEntry();
	}
	
	return FText::GetEmpty();
}

EVisibility SGridMapEditorToolkitWidget::GetVisibility_PaintOptions() const
{
	if (IsPaintTool())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE