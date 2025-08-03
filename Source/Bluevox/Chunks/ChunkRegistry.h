// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Position/RegionPosition.h"
#include "UObject/Object.h"
#include "ChunkRegistry.generated.h"

struct FRegionFile;
class UChunkData;
struct FChunkPosition;
class AChunk;
class URegion;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UChunkRegistry : public UObject
{
	GENERATED_BODY()

	friend class UVirtualMapTaskManager;
	
	TMap<FRegionPosition, TSharedPtr<FRegionFile>> Regions;

	FCriticalSection RegionsLock;

	UPROPERTY()
	TMap<const FChunkPosition, UChunkData*> ChunksData;

	FCriticalSection ChunksDataLock;

	UPROPERTY()
	TMap<const FChunkPosition, AChunk*> ChunkActors;
	
public:
	TSharedPtr<FRegionFile> LoadRegionFile(const FRegionPosition& Position);
	
	UFUNCTION()
	UChunkData* GetChunkData(const FChunkPosition& Position);

	UFUNCTION()
	UChunkData* FetchChunkDataFromDisk(const FChunkPosition& Position);
	
	UFUNCTION()
	bool HasChunkData(const FChunkPosition& Position);

	UFUNCTION()
	UChunkData* LoadChunkData(const FChunkPosition& Position);

	UFUNCTION()
	AChunk* GetChunkActor(const FChunkPosition& Position) const;
};
