#include "GridMapHelper.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
FName FGridMapHelper::NAME_GridMapActorInstanceTag(TEXT("GridMapActorInstance"));

void FGridMapHelper::SetTileset(AActor* InActor, const FName& TilesetName)
{
	if (InActor)
	{
		InActor->Tags.AddUnique(TilesetName);
	}
}

void FGridMapHelper::SetIsOwnedByGridMap(AActor* InActor, bool bOwned)
{
	
	if (InActor)
	{
		//FSetActorHiddenInSceneOutliner SetHidden(InActor, bOwned);
		if (bOwned)
		{
			InActor->Tags.AddUnique(NAME_GridMapActorInstanceTag);
		}
		else
		{
			InActor->Tags.Remove(NAME_GridMapActorInstanceTag);
		}
	}
}

bool FGridMapHelper::IsOwnedByGridMap(const AActor* InActor)
{
	return InActor != nullptr && InActor->ActorHasTag(NAME_GridMapActorInstanceTag);
}
#endif