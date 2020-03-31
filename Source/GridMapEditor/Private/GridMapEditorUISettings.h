#pragma once

#include "CoreMinimal.h"
#include "GridMapEditorTypes.h"

struct FGridMapEditorUISettings
{
public:
	FGridMapEditorUISettings();

	// tool
	bool GetPaintToolSelected() const { return bPaintToolSelected ? true : false; }
	void SetPaintToolSelected(bool InbPaintToolSelected) { bPaintToolSelected = InbPaintToolSelected; }

	bool GetSelectToolSelected() const { return bSelectToolSelected ? true : false; }
	void SetSelectToolSelected(bool bInSelectToolSelected) { bSelectToolSelected = bInSelectToolSelected; }

	bool GetSettingsToolSelected() const { return bSettingsToolSelected; }
	void SetSettingsToolSelected(bool bInSettingsToolSelected) { bSettingsToolSelected = bInSettingsToolSelected; }

	float GetPaintHeight() const { return PaintHeight; }
	void SetPaintHeight(float InHeight) { PaintHeight = InHeight; }

	EGridMapPaintMode GetPaintMode() const { return PaintMode; }
	void SetPaintMode(EGridMapPaintMode InPaintMode) { PaintMode = InPaintMode; }	

	bool GetHideOwnedActors() const { return bHideOwnedActors; }
	void SetHideOwnedActors(bool bInHideActors) { bHideOwnedActors = bInHideActors; }

	TWeakObjectPtr<class UGridMapTileSet> GetCurrentTileSet() const { return CurrentTileSetPtr; }
	void SetCurrentTileSet(class UGridMapTileSet* NewTileSet) { CurrentTileSetPtr = NewTileSet; }

private:
	bool bPaintToolSelected;
	bool bSelectToolSelected;
	bool bSettingsToolSelected;

	float PaintHeight;
	EGridMapPaintMode PaintMode;
	bool bHideOwnedActors;

	TWeakObjectPtr<class UGridMapTileSet> CurrentTileSetPtr;
	
};
