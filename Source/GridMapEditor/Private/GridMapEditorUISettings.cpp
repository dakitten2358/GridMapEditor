#include "GridMapEditorUISettings.h"

// defaults
FGridMapEditorUISettings::FGridMapEditorUISettings()
	: bPaintToolSelected(true)
	, bSelectToolSelected(false)
	, bSettingsToolSelected(false)
	, PaintOrigin(FVector::ZeroVector)
	, bHideOwnedActors(true)
	, bDebugDrawUpdatedTiles(false)
{}