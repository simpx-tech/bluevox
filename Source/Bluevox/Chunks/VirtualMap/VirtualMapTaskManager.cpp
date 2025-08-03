// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMapTaskManager.h"

#include "VirtualMap.h"
#include "Bluevox/Chunks/ChunkRegistry.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Network/ChunkDataNetworkPacket.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Tick/TickManager.h"

UVirtualMapTaskManager* UVirtualMapTaskManager::Init(const AGameManager* GameManager)
{
	VirtualMap = GameManager->VirtualMap;
	ChunkRegistry = GameManager->ChunkRegistry;
	TickManager = GameManager->TickManager;
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
				return ChunkRegistry->LoadChunkData(ChunkPosition);
			}, [ChunkPosition, this] (UChunkData* ChunkData)
			{
				if (PendingNetSend.Contains(ChunkPosition))
				{
					for (const auto& Player : PendingNetSend[ChunkPosition])
					{
						if (VirtualMap->VirtualChunks.Contains(ChunkPosition))
						{
							const auto Chunk = ChunkRegistry->GetChunkData(ChunkPosition);
							Player->PlayerNetwork->SendToClient(NewObject<UChunkDataNetworkPacket>()->Init(Chunk));
						}
					}
				}
				// DEV check if we can send this to any player (PendingNetSend)
				// DEV also schedule spawn actor and save to ChunkRegistry
			}, [this, ChunkPosition] (UChunkData* ChunkData)
			{
				return ProcessingLoad.FindRef(ChunkPosition) == true;
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
			const auto Chunk = ChunkRegistry->GetChunkData(ChunkPosition);
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
			ProcessingRender.Add(ChunkPosition, false);
		}
	}
}

TStatId UVirtualMapTaskManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMapTaskManager, STATGROUP_Tickables);
}

void UVirtualMapTaskManager::Tick(float DeltaTime)
{
	// DEV check if we can process the render tasks (?)
}
