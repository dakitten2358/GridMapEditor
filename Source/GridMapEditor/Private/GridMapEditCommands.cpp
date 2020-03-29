#include "GridMapEditCommands.h"
#include "EditorStyleSet.h"
#include "GridMapEditor.h"

#define LOCTEXT_NAMESPACE "GridMapEditor"

FGridMapEditCommands::FGridMapEditCommands()
	: TCommands<FGridMapEditCommands>(
		"GridMapEditCommands",	// context name for fast lookup
		NSLOCTEXT("GridMapEditor", "GridMapEditorMode", "Grid Map Edit Mode"), // localized text
		NAME_None,	// parent
		FGridMapEditorModule::StyleSetName)
{
}

void FGridMapEditCommands::RegisterCommands()
{
	MakeUICommand(SetPaintTiles, TEXT("PaintTiles"), LOCTEXT("Paint", "Paint"), LOCTEXT("PaintDescription", "Paint Tiles"), EUserInterfaceActionType::ToggleButton);
	/*
	UI_COMMAND(SetReapplySettings, "Reapply", "Reapply settings to instances", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetSelect, "Select", "Select", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetLassoSelect, "Lasso", "Lasso Select", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetPaintBucket, "Fill", "Paint Bucket", EUserInterfaceActionType::ToggleButton, FInputChord());
	*/
}

void FGridMapEditCommands::MakeUICommand(TSharedPtr<FUICommandInfo>& OutCommand, const TCHAR* CommandName, const FText&  FriendlyName, const FText& InDescription, const EUserInterfaceActionType CommandType, const FInputChord& InDefaultChord)
{
	FName IconName(*(GetContextName().ToString() + (FString(TEXT(".")) + CommandName)));
	FUICommandInfo::MakeCommandInfo(
		this->AsShared(),
		OutCommand,
		CommandName,
		FriendlyName,
		InDescription,
		FSlateIcon(this->GetStyleSetName(), IconName),
		CommandType,
		InDefaultChord
	);
}

#undef LOCTEXT_NAMESPACE