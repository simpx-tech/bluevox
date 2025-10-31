#include "EntityConversionSystem.h"

#include "EntityFacade.h"
#include "EntityTypes.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Tick/TickManager.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UEntityConversionSystem* UEntityConversionSystem::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	if (GameManager && GameManager->TickManager)
	{
		GameManager->TickManager->RegisterUObjectTickable(this);
	}
	return this;
}

void UEntityConversionSystem::GameTick(float DeltaTime)
{
	if (!GameManager || !GameManager->bServer) return;

	AccumulatedSeconds += DeltaTime;
	if (AccumulatedSeconds >= GameConstants::EntityConversion::IntervalSeconds)
	{
		AccumulatedSeconds = 0.0f;
		EvaluateConversions();
	}
}

void UEntityConversionSystem::EvaluateConversions()
{
	if (!GameManager || !GameManager->ChunkRegistry) return;

	// Gather player world locations on server
	TArray<FVector> PlayerLocations;
	for (FConstPlayerControllerIterator It = GameManager->GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;
		PlayerLocations.Add(Pawn->GetActorLocation());
	}
	if (PlayerLocations.Num() == 0) return;

	const float ToEntityCm = GameConstants::EntityConversion::ToEntityRangeCm;
	const float ToInstanceCm = GameConstants::EntityConversion::ToInstanceRangeCm;

	// Get chunks that are potentially within conversion range
	TArray<FChunkPosition> RelevantChunks = GetChunksWithinRange(PlayerLocations, ToInstanceCm);

	// Also include chunks that have converted entities (to check if they should be reverted)
	TSet<FChunkPosition> ChunksToProcess;
	for (const FChunkPosition& ChunkPos : RelevantChunks)
	{
		ChunksToProcess.Add(ChunkPos);
	}
	for (const auto& Pair : ConvertedEntities)
	{
		ChunksToProcess.Add(Pair.Key.ChunkPosition);
	}

	// Track which converted entities should remain converted
	TSet<FConvertedEntityKey> EntitiesThatShouldBeConverted;

	// Process each relevant chunk
	for (const FChunkPosition& ChunkPos : ChunksToProcess)
	{
		AChunk* Chunk = GameManager->ChunkRegistry->GetChunkActor(ChunkPos);
		if (!Chunk || !Chunk->ChunkData) continue;

		// Compute chunk bounds in world space
		const float XYSize = GameConstants::Scaling::XYWorldSize;
		const float ChunkSize = GameConstants::Chunk::Size;
		const FVector ChunkMin(ChunkPos.X * ChunkSize * XYSize,
		                       ChunkPos.Y * ChunkSize * XYSize,
		                       0.0f);

		// Check entities in this chunk
		TArray<int32> EntitiesToConvert;

		{
			FReadScopeLock ReadLock(Chunk->ChunkData->Lock);

			// Iterate through all entities in the chunk
			for (auto It = Chunk->ChunkData->Entities.CreateConstIterator(); It; ++It)
			{
				const int32 EntityArrayIndex = It.GetIndex();
				const FEntityRecord& Entity = *It;

				// Calculate world position of this entity
				FVector EntityWorldPos = ChunkMin + Entity.Transform.GetLocation();

				// Find closest player distance
				float MinDistToPlayer = TNumericLimits<float>::Max();
				for (const FVector& PlayerLoc : PlayerLocations)
				{
					float Dist = FVector::Dist(EntityWorldPos, PlayerLoc);
					MinDistToPlayer = FMath::Min(MinDistToPlayer, Dist);
				}

				FConvertedEntityKey Key(ChunkPos, EntityArrayIndex);
				bool bIsCurrentlyConverted = ConvertedEntities.Contains(Key);

				// Decide conversion based on distance and current state
				if (MinDistToPlayer <= ToEntityCm)
				{
					// Should be converted to entity
					if (!bIsCurrentlyConverted)
					{
						EntitiesToConvert.Add(EntityArrayIndex);
					}
					EntitiesThatShouldBeConverted.Add(Key);
				}
				else if (MinDistToPlayer >= ToInstanceCm && bIsCurrentlyConverted)
				{
					// Beyond reconversion distance and currently converted - will be reverted
					// (handled later by comparing sets)
				}
				// else: In hysteresis zone - maintain current state
			}
		}

		// Convert entities to facades if needed
		if (EntitiesToConvert.Num() > 0)
		{
			ConvertEntitiesToFacades(Chunk, EntitiesToConvert);
		}
	}

	// Find entities that should be converted back to instances
	TArray<FConvertedEntityKey> KeysToRevert;
	for (const auto& Pair : ConvertedEntities)
	{
		if (!EntitiesThatShouldBeConverted.Contains(Pair.Key))
		{
			KeysToRevert.Add(Pair.Key);
		}
	}

	// Convert facades back to instances
	if (KeysToRevert.Num() > 0)
	{
		ConvertFacadesToInstances(KeysToRevert);
	}
}

TArray<FChunkPosition> UEntityConversionSystem::GetChunksWithinRange(const TArray<FVector>& PlayerLocations, float RangeCm) const
{
	TArray<FChunkPosition> Result;
	if (PlayerLocations.Num() == 0) return Result;

	const float ChunkSize = GameConstants::Chunk::Size;
	const float XYSize = GameConstants::Scaling::XYWorldSize;
	const float ChunkSizeCm = ChunkSize * XYSize;

	// Calculate the number of chunks to check in each direction
	const int32 ChunkRadius = FMath::CeilToInt(RangeCm / ChunkSizeCm) + 1; // +1 for safety margin

	// Find all unique chunk positions within range of any player
	TSet<FChunkPosition> UniqueChunks;

	for (const FVector& PlayerLoc : PlayerLocations)
	{
		// Convert player position to chunk coordinates
		const int32 PlayerChunkX = FMath::FloorToInt(PlayerLoc.X / ChunkSizeCm);
		const int32 PlayerChunkY = FMath::FloorToInt(PlayerLoc.Y / ChunkSizeCm);

		// Check all chunks within the radius
		for (int32 DX = -ChunkRadius; DX <= ChunkRadius; ++DX)
		{
			for (int32 DY = -ChunkRadius; DY <= ChunkRadius; ++DY)
			{
				const FChunkPosition ChunkPos(PlayerChunkX + DX, PlayerChunkY + DY);

				// Calculate actual distance from player to chunk center
				const FVector2D ChunkCenter((ChunkPos.X + 0.5f) * ChunkSizeCm,
				                            (ChunkPos.Y + 0.5f) * ChunkSizeCm);
				const float DistToChunk = FVector2D::Distance(ChunkCenter, FVector2D(PlayerLoc.X, PlayerLoc.Y));

				// Only include chunks that are actually within range
				if (DistToChunk <= RangeCm + ChunkSizeCm * 0.707f) // Add diagonal distance for chunk corners
				{
					UniqueChunks.Add(ChunkPos);
				}
			}
		}
	}

	// Only return chunks that actually exist
	for (const FChunkPosition& ChunkPos : UniqueChunks)
	{
		if (GameManager->ChunkRegistry->GetChunkActor(ChunkPos))
		{
			Result.Add(ChunkPos);
		}
	}

	return Result;
}

void UEntityConversionSystem::ConvertEntitiesToFacades(AChunk* Chunk, const TArray<int32>& EntityIndices)
{
	if (!Chunk || !Chunk->ChunkData || !GameManager) return;

	const FChunkPosition& ChunkPos = Chunk->ChunkData->Position;
	const float XYSize = GameConstants::Scaling::XYWorldSize;
	const float ChunkSize = GameConstants::Chunk::Size;
	const FVector ChunkWorldOffset(ChunkPos.X * ChunkSize * XYSize,
	                               ChunkPos.Y * ChunkSize * XYSize,
	                               0.0f);

	for (int32 EntityArrayIndex : EntityIndices)
	{
		FWriteScopeLock WriteLock(Chunk->ChunkData->Lock);

		if (!Chunk->ChunkData->Entities.IsValidIndex(EntityArrayIndex))
			continue;

		FEntityRecord& Entity = Chunk->ChunkData->Entities[EntityArrayIndex];

		// Skip if already converted
		if (Entity.bIsConvertedToEntity)
			continue;

		// Resolve instance type data asset
		UInstanceTypeDataAsset* TypeAsset = nullptr;
		if (Entity.InstanceTypeId.IsValid())
		{
			UAssetManager& AssetManager = UAssetManager::Get();
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(Entity.InstanceTypeId);
			if (AssetPath.IsValid())
			{
				TypeAsset = Cast<UInstanceTypeDataAsset>(AssetManager.GetStreamableManager().LoadSynchronous(AssetPath, false));
			}
		}

		// Verify conversion is allowed and we have a valid entity class
		if (!TypeAsset)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[EntityConversion] Skipping conversion: missing type asset for %s (entity %d in chunk %s)"),
			       *Entity.InstanceTypeId.ToString(), EntityArrayIndex, *ChunkPos.ToString());
			continue;
		}
		if (!TypeAsset->bCanConvertToEntity)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[EntityConversion] Skipping conversion: bCanConvertToEntity=false for %s (entity %d in chunk %s)"),
			       *Entity.InstanceTypeId.ToString(), EntityArrayIndex, *ChunkPos.ToString());
			continue;
		}
		if (!TypeAsset->EntityClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EntityConversion] Skipping conversion: EntityClass not set for %s (entity %d in chunk %s)"),
			       *Entity.InstanceTypeId.ToString(), EntityArrayIndex, *ChunkPos.ToString());
			continue;
		}

		UClass* ClassToSpawn = TypeAsset->EntityClass.Get();
		if (!ClassToSpawn->IsChildOf(AEntityFacade::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EntityConversion] Skipping conversion: EntityClass %s is not a subclass of AEntityFacade for %s (entity %d in chunk %s)"),
			       *ClassToSpawn->GetName(), *Entity.InstanceTypeId.ToString(), EntityArrayIndex, *ChunkPos.ToString());
			continue;
		}

		// Spawn the facade actor
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FTransform WorldTransform = Entity.Transform;
		WorldTransform.AddToTranslation(ChunkWorldOffset);

		AEntityFacade* Facade = GameManager->GetWorld()->SpawnActor<AEntityFacade>(
			ClassToSpawn,
			WorldTransform,
			SpawnParams
		);

		if (Facade)
		{
			// Mark as converted only after successful spawn
			Entity.bIsConvertedToEntity = true;

			// Initialize the facade
			Facade->Init(GameManager);
			Facade->InstanceTypeId = Entity.InstanceTypeId;
			Facade->EntityIndex = EntityArrayIndex;
			Facade->ChunkPosition = ChunkPos;
			// TODO serialize facade with node data (?)
			// Facade->CustomData = Entity.CustomData;

			// Track the conversion
			FConvertedEntityKey Key(ChunkPos, EntityArrayIndex);
			ConvertedEntities.Add(Key, Facade);

			// Hide the instance on server
			if (Entity.InstanceIndex != INDEX_NONE)
			{
				UHierarchicalInstancedStaticMeshComponent* HISM = Chunk->GetInstanceComponent(Entity.InstanceTypeId);
				if (HISM)
				{
					// Hide instance by scaling to zero (preserves index)
					FTransform InstanceTransform;
					if (HISM->GetInstanceTransform(Entity.InstanceIndex, InstanceTransform, true))
					{
						InstanceTransform.SetScale3D(FVector::ZeroVector);
						HISM->UpdateInstanceTransform(Entity.InstanceIndex, InstanceTransform, true, true, true);
					}
				}
			}

			UE_LOG(LogTemp, Verbose, TEXT("[EntityConversion] Converted entity %d in chunk %s to facade (type %s)"),
			       EntityArrayIndex, *ChunkPos.ToString(), *Entity.InstanceTypeId.ToString());
		}
	}
}

void UEntityConversionSystem::ConvertFacadesToInstances(const TArray<FConvertedEntityKey>& KeysToRevert)
{
	if (!GameManager || !GameManager->ChunkRegistry) return;

	for (const FConvertedEntityKey& Key : KeysToRevert)
	{
		// Find and destroy the facade
		TWeakObjectPtr<AEntityFacade>* FacadePtr = ConvertedEntities.Find(Key);
		if (FacadePtr && FacadePtr->IsValid())
		{
			AEntityFacade* Facade = FacadePtr->Get();

			// Get the chunk
			AChunk* Chunk = GameManager->ChunkRegistry->GetChunkActor(Key.ChunkPosition);
			if (Chunk && Chunk->ChunkData)
			{
				FWriteScopeLock WriteLock(Chunk->ChunkData->Lock);

				if (Chunk->ChunkData->Entities.IsValidIndex(Key.EntityArrayIndex))
				{
					FEntityRecord& Entity = Chunk->ChunkData->Entities[Key.EntityArrayIndex];

					// Update entity transform from facade before destroying
					const float XYSize = GameConstants::Scaling::XYWorldSize;
					const float ChunkSize = GameConstants::Chunk::Size;
					const FVector ChunkWorldOffset(Key.ChunkPosition.X * ChunkSize * XYSize,
					                               Key.ChunkPosition.Y * ChunkSize * XYSize,
					                               0.0f);

					Entity.Transform = Facade->GetActorTransform();
					Entity.Transform.AddToTranslation(-ChunkWorldOffset); // Convert back to local
					Entity.bIsConvertedToEntity = false;

					// Restore the instance visibility
					if (Entity.InstanceIndex != INDEX_NONE)
					{
						UHierarchicalInstancedStaticMeshComponent* HISM = Chunk->GetInstanceComponent(Entity.InstanceTypeId);
						if (HISM)
						{
							// Restore instance transform (local/component space)
							HISM->UpdateInstanceTransform(Entity.InstanceIndex, Entity.Transform, false, true, true);
						}
					}
				}
			}

			// Destroy the facade actor
			Facade->Destroy();

			UE_LOG(LogTemp, Verbose, TEXT("[EntityConversion] Converted facade back to instance in chunk %s"),
			       *Key.ChunkPosition.ToString());
		}

		// Remove from tracking
		ConvertedEntities.Remove(Key);
	}
}