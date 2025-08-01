// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RegionFile.h"
#include "UObject/Object.h"
#include "ChunkRegistry.generated.h"

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

	UPROPERTY()
	TMap<FIntVector2, FRegionFile> Regions;

	UPROPERTY()
	TMap<FIntVector2, UChunkData*> ChunkData;

	UPROPERTY()
	TMap<FIntVector2, AChunk*> Chunks;
	
public:
	// DEV keep regions files and chunks get logic
	UFUNCTION()
	UChunkData* GetChunkData(const FChunkPosition& Position) const;

	UFUNCTION()
	UChunkData* GetChunkDataFromDisk(const FChunkPosition& Position);
	
	UFUNCTION()
	bool HasChunkData(const FChunkPosition& Position) const;

	UFUNCTION()
	UChunkData* LoadChunkData(const FChunkPosition& Position);

	UFUNCTION()
	AChunk* GetChunk(const FChunkPosition& Position) const;
};
