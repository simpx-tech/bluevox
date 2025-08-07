// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
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
		: bSuccess(false)
	{
	}
	
	bool bSuccess = false;
	TArray<FChunkColumn> Columns = {};
};

/**
 *
 */
UCLASS()
class BLUEVOX_API UVirtualMapTaskManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FChunkPosition, bool> ProcessingLoad;

	UPROPERTY()
	TSet<FChunkPosition> PendingRender;

	TMap<FChunkPosition, int32> ProcessingRender;

	UPROPERTY()
	int32 LastRenderIndex = 0;
	
	UPROPERTY()
	TMap<FChunkPosition, bool> ProcessingUnload;
	
	TMap<FChunkPosition, TArray<const AMainController*>> PendingNetSend;
	
	UPROPERTY()
	AGameManager* GameManager = nullptr;
public:
	UVirtualMapTaskManager* Init(AGameManager* InGameManager);
	
	UFUNCTION()
	void ScheduleLoad(const TSet<FChunkPosition>& ChunksToLoad);
	
	UFUNCTION()
	void ScheduleRender(const TSet<FChunkPosition>& ChunksToRender);

	UFUNCTION()
	void ScheduleNetSend(const AMainController* ToPlayer, const TSet<FChunkPosition>& ChunksToSend);

	UFUNCTION()
	void ScheduleUnload(const TSet<FChunkPosition>& ChunksToUnload);

	virtual TStatId GetStatId() const override;

	virtual void Tick(float DeltaTime) override;
};
