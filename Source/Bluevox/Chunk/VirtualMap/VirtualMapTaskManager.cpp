// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMapTaskManager.h"

#include "ParticleHelper.h"
#include "VirtualMap.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/RegionFile.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Data/RenderChunkPayload.h"
#include "Bluevox/Chunk/Position/LocalChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Network/ChunkDataNetworkPacket.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Tick/TickManager.h"
#include "DynamicMesh/DynamicMesh3.h"

UVirtualMapTaskManager* UVirtualMapTaskManager::Init(const AGameManager* GameManager)
{
	VirtualMap = GameManager->VirtualMap;
	ChunkRegistry = GameManager->ChunkRegistry;
	TickManager = GameManager->TickManager;
	WorldSave = GameManager->WorldSave;
	return this;
}

void UVirtualMapTaskManager::ScheduleLoad(const TSet<FChunkPosition>& ChunksToLoad)
{
	for (const auto& ChunkPosition : ChunksToLoad)
	{
		if (ProcessingLoad.Contains(ChunkPosition))
		{
			ProcessingLoad.Add(ChunkPosition, true);
		} else
		{
			TickManager->RunAsyncThen([this, ChunkPosition]
			{
				return ChunkRegistry->Th_LoadChunkData(ChunkPosition);
			}, [ChunkPosition, this] (UChunkData* ChunkData)
			{
				if (PendingNetSend.Contains(ChunkPosition))
				{
					if (VirtualMap->VirtualChunks.Contains(ChunkPosition))
					{
						const auto Chunk = ChunkRegistry->Th_GetChunkData(ChunkPosition);
						for (const auto& Player : PendingNetSend[ChunkPosition])
						{
							Player->PlayerNetwork->SendToClient(NewObject<UChunkDataNetworkPacket>()->Init(Chunk));
						}
					}
				}

				// Prevent spawn from stuttering game
				TickManager->ScheduleFn(
					[ChunkPosition, this]
					{
						if (!ProcessingUnload.Contains(ChunkPosition))
						{
							ChunkRegistry->SpawnChunk(ChunkPosition);	
						}
					});
			}, [this, ChunkPosition] (UChunkData* ChunkData)
			{
				return ProcessingLoad.FindRef(ChunkPosition) == true;
			}, [this, ChunkPosition] {
				ProcessingLoad.Remove(ChunkPosition);
			});
		}
		
		if (ProcessingUnload.Contains(ChunkPosition))
		{
			ProcessingUnload.Add(ChunkPosition, false);
		}
	}
}

void UVirtualMapTaskManager::ScheduleRender(const TSet<FChunkPosition>& ChunksToRender)
{
	for (const auto& ChunkPosition : ChunksToRender)
	{
		// Only add the ones that are not being unloaded
		if (!ProcessingUnload.Contains(ChunkPosition))
		{
			// Not render here, only render when all chunks are loaded
			// TODO probably there's a better way to handle this
			PendingRender.Add(ChunkPosition);
		}
	}
}

void UVirtualMapTaskManager::ScheduleNetSend(const AMainController* ToPlayer,
	const TSet<FChunkPosition>& ChunksToSend)
{
	for (const auto& ChunkPosition : ChunksToSend)
	{
		if (VirtualMap->VirtualChunks.Contains(ChunkPosition))
		{
			const auto Chunk = ChunkRegistry->Th_GetChunkData(ChunkPosition);
			ToPlayer->PlayerNetwork->SendToClient(NewObject<UChunkDataNetworkPacket>()->Init(Chunk));
		} else
		{
#if !UE_BUILD_SHIPPING
			if (!ProcessingLoad.Contains(ChunkPosition))
			{
				UE_LOG(LogTemp, Warning, TEXT("Chunk %s was requested for a NetSend, but it was not scheduled for load. Player: %s"),
					*ChunkPosition.ToString(), *ToPlayer->GetName());
			}
#endif

			if (!ProcessingUnload.Contains(ChunkPosition))
			{
				PendingNetSend.FindOrAdd(ChunkPosition).Add(ToPlayer);	
			}
		}
	}
}

void UVirtualMapTaskManager::ScheduleUnload(const TSet<FChunkPosition>& ChunksToUnload)
{
	for (const auto& ChunkPosition : ChunksToUnload)
	{
		if (ProcessingLoad.Contains(ChunkPosition))
		{
			ProcessingLoad.Add(ChunkPosition, false);
		}

		PendingRender.Remove(ChunkPosition);
		if (ProcessingRender.Contains(ChunkPosition))
		{
			ProcessingRender.Add(ChunkPosition, -1);
		}
		
		if (ProcessingUnload.Contains(ChunkPosition))
		{
			ProcessingUnload.Add(ChunkPosition, true);
			continue;
		}

		const auto RegionPosition = FRegionPosition::FromChunkPosition(ChunkPosition);
		const auto LocalChunkPosition = FLocalChunkPosition::FromChunkPosition(ChunkPosition);
		const auto Region = ChunkRegistry->Th_GetRegionFile(RegionPosition);
		TickManager->RunAsyncThen(
			[this, LocalChunkPosition, ChunkPosition, Region]
			{
				Region->Th_SaveChunk(LocalChunkPosition, ChunkRegistry->Th_GetChunkData(ChunkPosition));
			},
			[ChunkPosition, this]
			{
				ChunkRegistry->RemoveChunk(ChunkPosition);
			},
			[this, ChunkPosition]
			{
				return ProcessingUnload.FindRef(ChunkPosition) == true;
			},
			[this, ChunkPosition] {
				ProcessingUnload.Remove(ChunkPosition);
			}
		);
	}
}

TStatId UVirtualMapTaskManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMapTaskManager, STATGROUP_Tickables);
}

void UVirtualMapTaskManager::Tick(float DeltaTime)
{
	if (ProcessingLoad.Num() == 0 && PendingRender.Num() != 0)
	{
		TArray<FChunkPosition> ToRemove;
		for (const auto& ChunkPosition : PendingRender)
		{
			// Only start rendering when the chunk is loaded and spawned
			const auto Chunk = ChunkRegistry->GetChunkActor(ChunkPosition);
			if (!Chunk)
			{
				continue;
			}
			
			ProcessingRender.Add(ChunkPosition, true);
			ToRemove.Add(ChunkPosition);

			const auto State = VirtualMap->VirtualChunks.FindRef(ChunkPosition).State;

			const auto RenderId = LastRenderIndex++;
			FRenderChunkPayload Payload;
			Payload.Columns = Chunk->Data->Columns;
			Payload.bDirty = Chunk->Data->bDirty;
			Payload.State = State;
			// DEV Neighbors Payload.EastColumns = 
			
			TickManager->RunAsyncThen(
				[Chunk, Payload = MoveTemp(Payload)] mutable
				{
					UE::Geometry::FDynamicMesh3 Mesh;
					Chunk->BeginRender(MoveTemp(Payload), Mesh);
					return MoveTemp(Mesh);
				},
				[Chunk](UE::Geometry::FDynamicMesh3&& Mesh)
				{
					Chunk->CommitRender(MoveTemp(Mesh));
				},
				[this, ChunkPosition, RenderId](const UE::Geometry::FDynamicMesh3& Mesh)
				{
					return ProcessingRender.FindRef(ChunkPosition) == RenderId;
				},
				[this, ChunkPosition]
				{
					ProcessingRender.Remove(ChunkPosition);
				}
			);
		}

		for (int32 i = 0; i < ToRemove.Num(); ++i)
		{
			PendingRender.Remove(ToRemove[i]);
		}
	}
}
