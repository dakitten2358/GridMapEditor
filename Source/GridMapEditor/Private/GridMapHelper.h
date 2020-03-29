#pragma once

#include "CoreMinimal.h"

class FGridMapHelper 
{
public:
#if WITH_EDITOR
	static FName NAME_GridMapActorInstanceTag;

	static void SetTileset(class AActor* InActor, const FName& TilesetName);
	static void SetIsOwnedByGridMap(class AActor* InActor, bool bOwned = true);
	static bool IsOwnedByGridMap(const AActor* InActor);
#endif
};