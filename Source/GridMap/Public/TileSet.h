// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TileSet.generated.h"

USTRUCT()
struct GRIDMAP_API FGridMapTileBitset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	uint32 Bitset;
};

USTRUCT()
struct GRIDMAP_API FGridMapTileList
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FGridMapTileBitset TileAdjacency;

	UPROPERTY(EditAnywhere)
	FRotator Rotation;

	UPROPERTY(EditAnywhere, meta = (AllowAbstract))
	TArray<TAssetPtr<class UStaticMesh>> Tiles;

	TAssetPtr<class UStaticMesh> GetRandomTile() const;
};

/**
 * 
 */
UCLASS(BlueprintType)
class GRIDMAP_API UGridMapTileSet : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere )
	FName Name;

	UPROPERTY(EditAnywhere)
	TArray<FGridMapTileList> Tiles;

	const FGridMapTileList* FindTilesForAdjacency(uint32 bitmask) const;
};
