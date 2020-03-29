// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GridMapEditor.h"
#include "AssetToolsModule.h"
#include "GridMapEditCommands.h"
#include "GridMapEditorMode.h"
#include "GridMapTileSetAssetTypeActions.h"
#include "Interfaces/IPluginManager.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "TileBitsetCustomization.h"

#define LOCTEXT_NAMESPACE "FGridMapEditorModule"

const FName FGridMapEditorModule::StyleSetName("GridMapStyle");

const FVector2D Icon14x14(14.0f, 14.0f);
const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);
const FVector2D Icon512x512(512.0f, 512.0f);

// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
void FGridMapEditorModule::StartupModule()
{
	// Icons
	FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("GridMapEditor"))->GetContentDir() + "/";
	StyleSet = MakeShareable(new FSlateStyleSet(StyleSetName));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set("GridMapEditor.Tab", new FSlateImageBrush(ContentDir + TEXT("Icons/UIIcons/mode_40.png"), Icon40x40));
	StyleSet->Set("GridMapEditor.Tab.Small", new FSlateImageBrush(ContentDir + TEXT("Icons/UIIcons/mode_40.png"), Icon16x16));

	StyleSet->Set("GridMapEditCommands.PaintTiles", new FSlateImageBrush(ContentDir + TEXT("Icons/UIIcons/paint_40.png"), Icon20x20));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

	// Property Customization
	RegisterCustomPropertyTypeLayout("GridMapTileBitset", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTileBitsetCustomization::MakeInstance));

	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	GridMapAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("GridMapEditor")), LOCTEXT("GridMapAssetCategory", "Grid Map"));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FGridMapTileSetAssetTypeActions(GridMapAssetCategory)));

	// Modes
	FEditorModeRegistry::Get().RegisterMode<FGridMapEditorMode>(FGridMapEditorMode::EM_GridMapEditorModeId, LOCTEXT("GridMapEditorModeName", "GridMapEditorMode"), FSlateIcon("GridMapStyle", "GridMapEditor.Tab", "GridMapEditor.Tab.Small"), true);
	FGridMapEditCommands::Register();
}

// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
// we call this function before unloading the module.
void FGridMapEditorModule::ShutdownModule()
{	
	FEditorModeRegistry::Get().UnregisterMode(FGridMapEditorMode::EM_GridMapEditorModeId);

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all structures
		for (auto It = RegisteredPropertyTypes.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(*It);
			}
		}

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		StyleSet.Reset();
	}
}

void FGridMapEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}


void FGridMapEditorModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	RegisteredPropertyTypes.Add(PropertyTypeName);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGridMapEditorModule, GridMapEditor)