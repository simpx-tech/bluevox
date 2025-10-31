#include "EntityFacade.h"

#include "Net/UnrealNetwork.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Entity/EntityNode.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Entity/EntityTypes.h"
#include "Kismet/GameplayStatics.h"

AEntityFacade::AEntityFacade()
{
	bReplicates = true;
}

AEntityFacade* AEntityFacade::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	return this;
}

void AEntityFacade::BeginPlay()
{
	Super::BeginPlay();
}

void AEntityFacade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEntityFacade, InstanceTypeId);
	DOREPLIFETIME(AEntityFacade, EntityIndex);
	DOREPLIFETIME(AEntityFacade, ChunkPosition);
}

void AEntityFacade::PostNetInit()
{
	Super::PostNetInit();

	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));
	
	// Client-side: Hide the corresponding HISM instance when the entity is replicated from server
	if (!HasAuthority() && GameManager && GameManager->ChunkRegistry)
	{
		// Get the chunk this entity belongs to
		AChunk* Chunk = GameManager->ChunkRegistry->GetChunkActor(ChunkPosition);
		if (Chunk && EntityIndex != INDEX_NONE)
		{
			// Get the entity record to find the instance index
			UChunkData* ChunkData = Chunk->ChunkData;
			if (ChunkData)
			{
				FReadScopeLock ReadLock(ChunkData->Lock);
				if (ChunkData->Entities.IsValidIndex(EntityIndex))
				{
					const FEntityRecord& EntityRecord = ChunkData->Entities[EntityIndex];

					// Hide the instance in the HISM
					if (EntityRecord.InstanceIndex != INDEX_NONE)
					{
						Chunk->Cl_HideInstance(InstanceTypeId, EntityRecord.InstanceIndex);
					}
				}
			}
		}
	}
}

void AEntityFacade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!HasAuthority() && GameManager && GameManager->ChunkRegistry && EntityIndex != INDEX_NONE)
	{
		// Get the chunk this entity belongs to
		AChunk* Chunk = GameManager->ChunkRegistry->GetChunkActor(ChunkPosition);
		if (Chunk)
		{
			// Get the entity record to find the instance transform and index
			UChunkData* ChunkData = Chunk->ChunkData;
			if (ChunkData)
			{
				FReadScopeLock ReadLock(ChunkData->Lock);
				if (ChunkData->Entities.IsValidIndex(EntityIndex))
				{
					const FEntityRecord& EntityRecord = ChunkData->Entities[EntityIndex];

					// Restore the instance in the HISM
					if (EntityRecord.InstanceIndex != INDEX_NONE)
					{
						Chunk->Cl_ShowInstance(InstanceTypeId, EntityRecord.InstanceIndex, EntityRecord.Transform);
					}
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}
