// Fill out your copyright notice in the Description page of Project Settings.


#include "TileSet.h"

UGridMapTileSet::UGridMapTileSet()
	: TileSize(100)
	, TileHeight(200)
{
}

TAssetPtr<class UStaticMesh> FGridMapTileList::GetRandomTile() const
{
	if (Tiles.Num() == 0)
		return TAssetPtr<class UStaticMesh>();

	int32 RandomElementIndex = FMath::RandRange(0, Tiles.Num() - 1);
	return Tiles[RandomElementIndex];
}

const FGridMapTileList* UGridMapTileSet::FindTilesForAdjacency(uint32 bitmask) const
{
	for (int i = 0; i < Tiles.Num(); ++i)
	{
		int32 tileBitmask = Tiles[i].TileAdjacency.Bitset;
		if (tileBitmask == bitmask)
			return &Tiles[i];
	}

	return nullptr;
}