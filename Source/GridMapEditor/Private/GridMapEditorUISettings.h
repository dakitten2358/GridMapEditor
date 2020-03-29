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
	float PaintHeight;
	EGridMapPaintMode PaintMode;
	bool bHideOwnedActors;

	TWeakObjectPtr<class UGridMapTileSet> CurrentTileSetPtr;
	
};
