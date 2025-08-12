// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UObject/Object.h"
#include "VirtualMapTaskManager.generated.h"

class UWorldSave;
class UTickManager;
class AGameManager;
class UChunkRegistry;
class UChunkData;
class UVirtualMap;
class AMainController;

struct FLoadResult
{
	FLoadResult()
	{
	}
	
	bool bSuccess = false;
	TArray<FChunkColumn> Columns = {};
};

struct FRenderResult
{
	FRenderResult()
	{
	}
	
	bool bSuccess = false;
	UE::Geometry::FDynamicMesh3 Mesh;
};

USTRUCT(BlueprintType)
struct FProcessingRender
{
	GENERATED_BODY()

	UPROPERTY()
	int32 LastRenderIndex = -1;

	UPROPERTY()
	int32 PendingTasks = 0;
};

USTRUCT(BlueprintType)
struct FPendingNetSendChunks
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FChunkPosition> ToSend;

	UPROPERTY()
	TSet<FChunkPosition> WaitingFor;

	UPROPERTY()
	const AMainController* Player = nullptr;
};

/**
 *
 */
UCLASS()
class BLUEVOX_API UVirtualMapTaskManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class UChunkDataNetworkPacket;

	UPROPERTY()
	TMap<FChunkPosition, bool> ProcessingLoad;

	UPROPERTY()
	TSet<FChunkPosition> PendingRender;

	UPROPERTY()
	TMap<FChunkPosition, FProcessingRender> ProcessingRender;

	UPROPERTY()
	TSet<FChunkPosition> WaitingToBeSent;

	UPROPERTY()
	bool bServer;
	
	UPROPERTY()
	int32 LastRenderIndex = 0;
	
	UPROPERTY()
	TMap<FChunkPosition, bool> ProcessingUnload;

	/**
	 * List of chunks together which are pending to be sent to the client.
	 */
	TSparseArray<FPendingNetSendChunks> PendingNetSendPackets;

	/**
	 * Mapping from the chunk position to a list of pending net packets is an array
	 * because a chunk can be requested by multiple players.
	 */
	TMap<FChunkPosition, TArray<int32>> PendingNetSendPacketByPosition;
	
	UPROPERTY()
	AGameManager* GameManager = nullptr;

	void Sv_ProcessPendingNetSend(FPendingNetSendChunks&& PendingNetSend) const;
	
public:
	UVirtualMapTaskManager* Init(AGameManager* InGameManager);
	
	UFUNCTION()
	void ScheduleLoad(const TSet<FChunkPosition>& ChunksToLoad);
	
	UFUNCTION()
	void ScheduleRender(const TSet<FChunkPosition>& ChunksToRender);

	UFUNCTION()
	void Sv_ScheduleNetSend(const AMainController* ToPlayer, const TSet<FChunkPosition>& ChunksToSend);

	UFUNCTION()
	void ScheduleUnload(const TSet<FChunkPosition>& ChunksToUnload);

	virtual TStatId GetStatId() const override;

	virtual void Tick(float DeltaTime) override;
};
