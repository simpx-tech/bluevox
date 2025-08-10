// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Position/RegionPosition.h"
#include "Position/ChunkPosition.h"
#include "UObject/Object.h"
#include "ChunkRegistry.generated.h"

struct FColumnPosition;
struct FChunkColumn;
class UWorldSave;
class AGameManager;
struct FRegionFile;
class UChunkData;
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

	TMap<FRegionPosition, int32> LoadedByRegion;

	FRWLock RegionsLock;

	UPROPERTY()
	AGameManager* GameManager;
	
	UPROPERTY()
	TMap<FChunkPosition, UChunkData*> ChunksData;

	FRWLock ChunksDataLock;

	UPROPERTY()
	TMap<FChunkPosition, AChunk*> ChunkActors;
	
public:
	UChunkRegistry* Init(AGameManager* InGameManager);
	
	TSharedPtr<FRegionFile> Th_LoadRegionFile(const FRegionPosition& Position);

	TSharedPtr<FRegionFile> Th_GetRegionFile(const FRegionPosition& Position);

	AChunk* SpawnChunk(FChunkPosition Position);

	FChunkColumn& Th_GetColumn(const FColumnPosition& GlobalColPosition);

	void UnregisterChunk(const FChunkPosition& Position);

	void LockForRender(const FChunkPosition& Position);

	void ReleaseForRender(const FChunkPosition& Position);
	
	UFUNCTION()
	UChunkData* Th_GetChunkData(const FChunkPosition& Position);

	UFUNCTION()
	void Th_RegisterChunk(const FChunkPosition& Position, UChunkData* Data);
	
	UFUNCTION()
	bool Th_FetchChunkDataFromDisk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns);
	
	UFUNCTION()
	bool Th_HasChunkData(const FChunkPosition& Position);

	UFUNCTION()
	AChunk* GetChunkActor(const FChunkPosition& Position) const;
};
