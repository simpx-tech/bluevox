// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VirtualChunk.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Chunk/Position/RegionPosition.h"
#include "UObject/Object.h"
#include "VirtualMap.generated.h"

class UVirtualMapTaskManager;
class AMainController;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UVirtualMap : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class UVirtualMapTaskManager;

	UPROPERTY()
	TMap<const AMainController*, FChunkPosition> ChunkPositionByPlayer;

	UPROPERTY()
	TMap<FChunkPosition, FVirtualChunk> VirtualChunks;
	
	UPROPERTY()
	TMap<FRegionPosition, int32> ChunksByRegion;

	UPROPERTY()
	UVirtualMapTaskManager* TaskManager;

	UFUNCTION()
	void RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& LiveChunks, const TSet<FChunkPosition>& FarChunks);

	UFUNCTION()
	void AddPlayerToChunks(const AMainController* Controller, const TSet<FChunkPosition>& LiveChunks, const TSet<FChunkPosition>& FarChunks);

	UFUNCTION()
	void HandlePlayerMovement(const AMainController* Controller, const FChunkPosition& OldPosition, const FChunkPosition& NewPosition);

	UPROPERTY()
	class AGameManager* GameManager = nullptr;
	
public:
	UVirtualMap* Init(AGameManager* InGameManager);

	void RegisterPlayer(const AMainController* Player);

	void UnregisterPlayer(const AMainController* Player);

	void UpdateFarDistanceForPlayer(const AMainController* Player, const int32 OldFarDistance, const int32 NewFarDistance);

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;
};
