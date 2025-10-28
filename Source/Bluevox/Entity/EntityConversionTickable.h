// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Tick/GameTickable.h"
#include "UObject/NoExportTypes.h"
#include "EntityConversionTickable.generated.h"

class AChunk;
class AGameManager;
class UChunkRegistry;
class UInstanceTypeDataAsset;
class AEntityFacade;
class UEntityNode;
struct FPrimaryAssetId;

/**
 * Tickable object that manages conversion of instances to entities
 * Server: Converts instances within range of players
 * Client: Hides instances when corresponding entities are replicated
 */
UCLASS()
class BLUEVOX_API UEntityConversionTickable : public UObject, public IGameTickable
{
	GENERATED_BODY()

private:
	UPROPERTY()
	AGameManager* GameManager = nullptr;

	UPROPERTY()
	UChunkRegistry* ChunkRegistry = nullptr;

	// Cache of loaded instance type assets
	TMap<FPrimaryAssetId, UInstanceTypeDataAsset*> LoadedAssets;

	// Time accumulator for update throttling
	float TimeSinceLastUpdate = 0.0f;

	// Update frequency
	static constexpr float UpdateInterval = 0.1f; // 10 updates per second

public:
	// Initialize the tickable
	void Init(AGameManager* InGameManager, UChunkRegistry* InChunkRegistry);

	// IGameTickable interface
	virtual void GameTick(float DeltaTime) override;

private:
	// Server-side logic
	void ServerTick_ConvertInstances();

	// Client-side logic
	void ClientTick_UpdateInstanceVisibility();

	// Process entity conversions for a specific chunk
	void ProcessChunkEntityConversions(AChunk* Chunk, const FVector& PlayerPos);

	// Spawn entity from an instance
	void SpawnEntityFromInstance(AChunk* Chunk, const FPrimaryAssetId& AssetId,
	                             int32 InstanceIndex, UInstanceTypeDataAsset* Asset);

	// Get maximum conversion distance from all loaded assets
	float GetMaxConversionDistance() const;

	// Load or get cached asset
	UInstanceTypeDataAsset* GetOrLoadAsset(const FPrimaryAssetId& AssetId);

	// Get chunks that could have convertible instances (including neighbors)
	TArray<AChunk*> GetChunksForConversion(const FVector& PlayerPos, float MaxDistance);

	// Check if we're on server
	bool IsServer() const;

	// Check if we're client-only
	bool IsClientOnly() const;
};