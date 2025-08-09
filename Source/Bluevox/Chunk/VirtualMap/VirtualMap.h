// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VirtualChunk.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
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
	UVirtualMapTaskManager* TaskManager;

	static EChunkState CalculateState(const FVirtualChunk& Chunk)
	{
		if (EnumHasAnyFlags(Chunk.State, EChunkState::LocalLive))
		{
			return EChunkState::Live | EChunkState::LocalLive;
		}

		if (Chunk.LiveForRemotePlayersAmount > 0)
		{
			return EChunkState::RemoteLive
				// Effectively turning into a live chunk (Collision + Rendered)
				| (EnumHasAnyFlags(Chunk.State, EChunkState::LocalFar) ? EChunkState::Rendered : EChunkState::None)
				// We should keep the local far state if it was set
				| Chunk.State & EChunkState::LocalFar;
		}

		if (EnumHasAnyFlags(Chunk.State, EChunkState::LocalFar))
		{
			return EChunkState::VisualOnly | EChunkState::LocalFar;
		}

		return EChunkState::RemoteVisualOnly;
	}

	UFUNCTION()
	void RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& LiveChunks, const TSet<FChunkPosition>& FarChunks);

	UFUNCTION()
	void AddPlayerToChunks(const AMainController* Controller, const TSet<FChunkPosition>& NewLiveChunks, const TSet<FChunkPosition>& NewFarChunks, const TSet<FChunkPosition>& FarToLive, const TSet<FChunkPosition>& LiveToFar);

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
