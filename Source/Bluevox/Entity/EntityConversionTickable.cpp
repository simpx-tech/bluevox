// Fill out your copyright notice in the Description page of Project Settings.


#include "EntityConversionTickable.h"
#include "EntityFacade.h"
#include "EntityNode.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameConstants.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UEntityConversionTickable::Init(AGameManager* InGameManager, UChunkRegistry* InChunkRegistry)
{
	GameManager = InGameManager;
	ChunkRegistry = InChunkRegistry;
}

void UEntityConversionTickable::GameTick(float DeltaTime)
{
	if (!GameManager || !ChunkRegistry)
	{
		return;
	}

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceLastUpdate = 0.0f;

	if (IsServer())
	{
		ServerTick_ConvertInstances();
	}
	else if (IsClientOnly())
	{
		ClientTick_UpdateInstanceVisibility();
	}
}

void UEntityConversionTickable::ServerTick_ConvertInstances()
{
	// Get all player positions
	TArray<FVector> PlayerPositions;
	for (auto It = GameManager->GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				PlayerPositions.Add(Pawn->GetActorLocation());
			}
		}
	}

	if (PlayerPositions.Num() == 0)
	{
		return;
	}

	// Get maximum conversion distance
	const float MaxDistance = GetMaxConversionDistance();
	if (MaxDistance <= 0.0f)
	{
		return;
	}

	// Process chunks near players
	TSet<AChunk*> ProcessedChunks;
	for (const FVector& PlayerPos : PlayerPositions)
	{
		TArray<AChunk*> NearbyChunks = GetChunksForConversion(PlayerPos, MaxDistance);

		for (AChunk* Chunk : NearbyChunks)
		{
			if (!ProcessedChunks.Contains(Chunk))
			{
				ProcessChunkEntityConversions(Chunk, PlayerPos);
				ProcessedChunks.Add(Chunk);
			}
		}
	}
}

void UEntityConversionTickable::ClientTick_UpdateInstanceVisibility()
{
	// Get visible chunks
	const auto& ChunkActors = ChunkRegistry->GetChunkActors();

	for (const auto& [Position, ChunkActor] : ChunkActors)
	{
		if (ChunkActor && ChunkActor->WasRecentlyRendered())
		{
			// Refresh instance visibility based on replicated entities
			ChunkActor->RefreshInstanceVisibility();
		}
	}
}

void UEntityConversionTickable::ProcessChunkEntityConversions(AChunk* Chunk, const FVector& PlayerPos)
{
	if (!Chunk || !Chunk->ChunkData)
	{
		return;
	}

	FWriteScopeLock WriteLock(Chunk->ChunkData->Lock);

	for (auto& [AssetId, Collection] : Chunk->ChunkData->InstanceCollections)
	{
		UInstanceTypeDataAsset* Asset = GetOrLoadAsset(AssetId);
		if (!Asset || !Asset->bCanConvertToEntity || !Asset->EntityClass)
		{
			continue;
		}

		for (int32 i = 0; i < Collection.Instances.Num(); i++)
		{
			// Skip already converted instances
			if (Collection.ConvertedInstanceIndices.Contains(i))
			{
				continue;
			}

			// Calculate world position
			const FVector WorldPos = Chunk->GetActorLocation() +
				Collection.Instances[i].Transform.GetLocation();

			// Check if within conversion distance of any player
			bool bShouldConvert = false;
			for (const FVector& CheckPos : {PlayerPos}) // Can expand to check all players
			{
				const float Distance = FVector::Dist(CheckPos, WorldPos);
				if (Distance <= Asset->EntityConversionDistance)
				{
					bShouldConvert = true;
					break;
				}
			}

			if (bShouldConvert)
			{
				SpawnEntityFromInstance(Chunk, AssetId, i, Asset);
			}
		}
	}
}

void UEntityConversionTickable::SpawnEntityFromInstance(AChunk* Chunk, const FPrimaryAssetId& AssetId,
                                                        int32 InstanceIndex, UInstanceTypeDataAsset* Asset)
{
	if (!Chunk || !Chunk->ChunkData || !Asset || !Asset->EntityClass)
	{
		return;
	}

	auto* Collection = Chunk->ChunkData->InstanceCollections.Find(AssetId);
	if (!Collection || !Collection->Instances.IsValidIndex(InstanceIndex))
	{
		return;
	}

	const FInstanceData& Instance = Collection->Instances[InstanceIndex];
	const FTransform WorldTransform(
		Instance.Transform.GetRotation(),
		Chunk->GetActorLocation() + Instance.Transform.GetLocation(),
		Instance.Transform.GetScale3D()
	);

	// Spawn the entity facade actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UWorld* World = GameManager->GetWorld();
	AEntityFacade* Facade = World->SpawnActor<AEntityFacade>(
		Asset->EntityClass,
		WorldTransform.GetLocation(),
		WorldTransform.Rotator(),
		SpawnParams
	);

	if (Facade)
	{
		// Set scale
		Facade->SetActorScale3D(WorldTransform.GetScale3D());

		// Notify entity it was spawned from instance
		Facade->OnSpawnedFromInstance(Instance);

		// Create backend node if it has one (server only)
		if (IsServer())
		{
			// The entity class should specify its node class
			// For now, we'll let the entity handle its own node creation
			// This could be expanded to support data-driven node classes
		}

		// Register in chunk's entity grid
		Chunk->ChunkData->RegisterEntity(Instance.Transform.GetLocation(), Facade);

		// Mark instance as converted
		Collection->ConvertedInstanceIndices.Add(InstanceIndex);

		// Update instance rendering
		Chunk->UpdateInstanceRendering();
	}
}

float UEntityConversionTickable::GetMaxConversionDistance() const
{
	float MaxDistance = 0.0f;

	for (const auto& [AssetId, Asset] : LoadedAssets)
	{
		if (Asset && Asset->bCanConvertToEntity)
		{
			MaxDistance = FMath::Max(MaxDistance, Asset->EntityConversionDistance);
		}
	}

	return MaxDistance;
}

UInstanceTypeDataAsset* UEntityConversionTickable::GetOrLoadAsset(const FPrimaryAssetId& AssetId)
{
	if (UInstanceTypeDataAsset** CachedAsset = LoadedAssets.Find(AssetId))
	{
		return *CachedAsset;
	}

	// Try to load the asset
	UAssetManager& AssetManager = UAssetManager::Get();
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);

	if (AssetPath.IsValid())
	{
		UInstanceTypeDataAsset* LoadedAsset = Cast<UInstanceTypeDataAsset>(AssetPath.TryLoad());
		if (LoadedAsset)
		{
			LoadedAssets.Add(AssetId, LoadedAsset);
			return LoadedAsset;
		}
	}

	return nullptr;
}

TArray<AChunk*> UEntityConversionTickable::GetChunksForConversion(const FVector& PlayerPos, float MaxDistance)
{
	TArray<AChunk*> ResultChunks;

	// Calculate chunk radius needed
	const float ChunkWorldSize = GameConstants::Chunk::Size * GameConstants::Scaling::XYWorldSize;
	const int32 ChunkRadius = FMath::CeilToInt(MaxDistance / ChunkWorldSize) + 1; // +1 for safety

	// Get player's chunk position
	const FChunkPosition PlayerChunkPos(
		FMath::FloorToInt(PlayerPos.X / ChunkWorldSize),
		FMath::FloorToInt(PlayerPos.Y / ChunkWorldSize)
	);

	// Get all chunks within radius (including neighbors)
	for (int32 X = -ChunkRadius; X <= ChunkRadius; X++)
	{
		for (int32 Y = -ChunkRadius; Y <= ChunkRadius; Y++)
		{
			FChunkPosition CheckPos(PlayerChunkPos.X + X, PlayerChunkPos.Y + Y);

			if (AChunk* Chunk = ChunkRegistry->GetChunkActor(CheckPos))
			{
				// Optional: Additional distance check for corner chunks
				const FVector ChunkCenter = Chunk->GetActorLocation() +
					FVector(ChunkWorldSize * 0.5f, ChunkWorldSize * 0.5f, 0);
				const float DistToChunk = FVector::Dist2D(PlayerPos, ChunkCenter);

				if (DistToChunk <= MaxDistance + ChunkWorldSize)
				{
					ResultChunks.Add(Chunk);
				}
			}
		}
	}

	return ResultChunks;
}

bool UEntityConversionTickable::IsServer() const
{
	return GameManager && GameManager->GetWorld() &&
		   GameManager->GetWorld()->GetNetMode() < NM_Client;
}

bool UEntityConversionTickable::IsClientOnly() const
{
	return GameManager && GameManager->IsClientOnly();
}