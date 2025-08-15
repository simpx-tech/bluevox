// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VirtualChunk.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "UObject/Object.h"
#include "VirtualMap.generated.h"

class AMainController;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UVirtualMap : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class UVirtualMapTaskManager;
	friend class UChunkDataNetworkPacket;

	UPROPERTY()
	TMap<const AMainController*, FChunkPosition> ChunkPositionByPlayer;

	UPROPERTY()
	TMap<FChunkPosition, FVirtualChunk> VirtualChunks;

	UFUNCTION()
	void RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& ToRemoveLoad, const TSet<FChunkPosition>& ToRemoveLive);

	UFUNCTION()
	void AddPlayerToChunks(const AMainController* Controller, const TSet<FChunkPosition>& ToLoad, const TSet<FChunkPosition>& ToLive);

	UFUNCTION()
	void HandleStateUpdate(const AMainController* Controller, const TSet<FChunkPosition>& LoadToLive, const TSet<FChunkPosition>& LiveToLoad);
	
	UFUNCTION()
	void HandlePlayerMovement(const AMainController* Controller, const FChunkPosition& OldPosition, const FChunkPosition& NewPosition);

	UPROPERTY()
	class AGameManager* GameManager = nullptr;

	UPROPERTY()
	bool bServer = false;

	UPROPERTY()
	TMap<const AMainController*, int32> WaitingSpawnRenderPerPlayer;

	TMap<FChunkPosition, TArray<const AMainController*>> PlayersWaitingForSpawnRender;
	
public:
	UPROPERTY()
	UVirtualMapTaskManager* TaskManager;
	
	UVirtualMap* Init(AGameManager* InGameManager);

	void RegisterPlayer(const AMainController* Player);

	void UnregisterPlayer(const AMainController* Player);

	void Sv_UpdateFarDistanceForPlayer(const AMainController* Player, const int32 OldFarDistance, const int32 NewFarDistance);

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	virtual ETickableTickType GetTickableTickType() const override;
};
