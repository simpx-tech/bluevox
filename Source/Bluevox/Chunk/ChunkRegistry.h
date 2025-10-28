// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Position/RegionPosition.h"
#include "Position/ChunkPosition.h"
#include "UObject/Object.h"
#include "ChunkRegistry.generated.h"

struct FPiece;
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

	friend class UChunkTaskManager;
	
	TMap<FRegionPosition, TSharedPtr<FRegionFile>> Regions;

	UPROPERTY()
	TMap<FRegionPosition, int32> LoadedByRegion;

	FRWLock RegionsLock;

	UPROPERTY()
	bool bServer = false;

	UPROPERTY()
	AGameManager* GameManager;

	UPROPERTY()
	TMap<FChunkPosition, UChunkData*> ChunksData;

	FRWLock ChunksDataLock;

	UPROPERTY()
	TMap<FChunkPosition, AChunk*> ChunkActors;

	FCriticalSection ChunksMarkedForUseLock;
	
	UPROPERTY()
	TMap<FChunkPosition, uint16> ChunksMarkedForUse;
  
	UPROPERTY()
	TSet<FChunkPosition> ChunksScheduledToRemove;

	UFUNCTION()
	void Th_RegisterChunk(const FChunkPosition& Position, UChunkData* Data);

	TSharedPtr<FRegionFile> Th_LoadRegionFile(const FRegionPosition& Position);
	
	void Th_UnregisterChunk(const FChunkPosition& Position);

	AChunk* SpawnChunk(FChunkPosition Position);

	void LockForRender(const FChunkPosition& Position);

	void UnlockForRender(const FChunkPosition& Position);
	
public:
	UChunkRegistry* Init(AGameManager* InGameManager);

	TSharedPtr<FRegionFile> Th_GetRegionFile(const FRegionPosition& Position);

	FChunkColumn& Th_GetColumn(const FColumnPosition& GlobalColPosition);

	void SetPiece(const FGlobalPosition& GlobalPosition, FPiece&& InPiece);

	void SetPiece(const FGlobalPosition& GlobalPosition, const FPiece& Piece);

	UFUNCTION()
	UChunkData* Th_GetChunkData(const FChunkPosition& Position);

	// TODO should not use Th_LoadRegionFile, instead use a Th_GetRegionFile and discard immediately
	UFUNCTION()
	bool Th_FetchChunkDataFromDisk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
	                                TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances);
	
	UFUNCTION()
	bool Th_HasChunkData(const FChunkPosition& Position);

public:
	// Get all chunk actors
	UFUNCTION(BlueprintPure, Category = "Chunk")
	const TMap<FChunkPosition, AChunk*>& GetChunkActors() const { return ChunkActors; }

	// Get specific chunk actor
	UFUNCTION(BlueprintPure, Category = "Chunk")
	AChunk* GetChunkActor(const FChunkPosition& Position) const;
};
