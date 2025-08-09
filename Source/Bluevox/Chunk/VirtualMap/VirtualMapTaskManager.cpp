// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMapTaskManager.h"

#include "VirtualMap.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/RegionFile.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/LocalChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Network/ChunkDataNetworkPacket.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Tick/TickManager.h"
#include "DynamicMesh/DynamicMesh3.h"

UVirtualMapTaskManager* UVirtualMapTaskManager::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	return this;
}

void UVirtualMapTaskManager::ScheduleLoad(const TSet<FChunkPosition>& ChunksToLoad)
{
	for (const auto& ChunkPosition : ChunksToLoad)
	{
		UE_LOG(LogChunk, Verbose, TEXT("Scheduling load for chunk %s"), *ChunkPosition.ToString());
		if (ProcessingLoad.Contains(ChunkPosition))
		{
			ProcessingLoad.Add(ChunkPosition, true);
		} else
		{
			ProcessingLoad.Add(ChunkPosition, true);
			
			GameManager->TickManager->RunAsyncThen([this, ChunkPosition]
			{
				FLoadResult LoadResult;
				LoadResult.bSuccess = GameManager->ChunkRegistry->Th_FetchChunkDataFromDisk(ChunkPosition, LoadResult.Columns);

				if (!LoadResult.bSuccess)
				{
					// TODO would re-generate chunk if fail to load from disk, is that a good idea?
					GameManager->WorldSave->WorldGenerator->GenerateChunk(ChunkPosition, LoadResult.Columns);
				}
				
				return MoveTemp(LoadResult);
			}, [ChunkPosition, this] (FLoadResult&& Result)
			{
				UE_LOG(LogChunk, Verbose, TEXT("Processing load for chunk %s"), *ChunkPosition.ToString());

				if (ProcessingLoad.FindRef(ChunkPosition) == true)
				{
					const auto ChunkData = NewObject<UChunkData>();
					ChunkData->Columns = MoveTemp(Result.Columns);
					GameManager->ChunkRegistry->Th_RegisterChunk(ChunkPosition, ChunkData);

					if (PendingNetSend.Contains(ChunkPosition))
					{
						UE_LOG(LogChunk, Verbose, TEXT("Chunk %s has PendingNetSend"), *ChunkPosition.ToString());
						if (
							ensureMsgf(GameManager->VirtualMap->VirtualChunks.Contains(ChunkPosition), TEXT("Chunk %s was not registered in VirtualMap when trying to send data to players."), *ChunkPosition.ToString())
							)
						{
							for (const auto& Player : PendingNetSend[ChunkPosition])
							{
								Player->PlayerNetwork->SendToClient
								(NewObject<UChunkDataNetworkPacket>()->Init(ChunkData));
							}
						}
					}

					// Prevent spawn from stuttering game
					GameManager->TickManager->Th_ScheduleFn(
						[ChunkPosition, this]
						{
							if (!ProcessingUnload.Contains(ChunkPosition))
							{
								GameManager->ChunkRegistry->SpawnChunk(ChunkPosition);	
							}
						});
				}

				if (ProcessingLoad.FindRef(ChunkPosition) == false)
				{
					GameManager->ChunkRegistry->UnregisterChunk(ChunkPosition);
				}
				
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
		if (GameManager->VirtualMap->VirtualChunks.Contains(ChunkPosition))
		{
			const auto Chunk = GameManager->ChunkRegistry->Th_GetChunkData(ChunkPosition);
			ToPlayer->PlayerNetwork->SendToClient(NewObject<UChunkDataNetworkPacket>()->Init(Chunk));
		} else
		{
#if !UE_BUILD_SHIPPING
			if (!ProcessingLoad.Contains(ChunkPosition))
			{
				UE_LOG(LogChunk, Warning, TEXT("Chunk %s was requested for a NetSend, but it was not scheduled for load. Player: %s"),
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
			UE_LOG(LogChunk, Verbose, TEXT("Cancelling load for chunk %s"), *ChunkPosition.ToString());
			ProcessingLoad.Add(ChunkPosition, false);
		}

		PendingRender.Remove(ChunkPosition);
		if (ProcessingRender.Contains(ChunkPosition))
		{
			ProcessingRender.Find(ChunkPosition)->LastRenderIndex = -1;
		}
		
		if (ProcessingUnload.Contains(ChunkPosition))
		{
			ProcessingUnload.Add(ChunkPosition, true);			
			continue;
		}

		ProcessingUnload.Add(ChunkPosition, true);

		const auto RegionPosition = FRegionPosition::FromChunkPosition(ChunkPosition);
		const auto LocalChunkPosition = FLocalChunkPosition::FromChunkPosition(ChunkPosition);
		const auto Region = GameManager->ChunkRegistry->Th_GetRegionFile(RegionPosition);

		if (!Region)
		{
			UE_LOG(LogChunk, Warning, TEXT("Failed to unload chunk %s: region file not found."), *ChunkPosition.ToString());
			continue;
		}

		const auto ChunkData = GameManager->ChunkRegistry->Th_GetChunkData(ChunkPosition);
		GameManager->TickManager->RunAsyncThen(
			[this, LocalChunkPosition, ChunkData, Region]
			{
				Region->Th_SaveChunk(LocalChunkPosition, ChunkData);
			},
			[ChunkPosition, this]
			{
				UE_LOG(LogChunk, Verbose, TEXT("Processing unload for chunk %s"), *ChunkPosition.ToString());
				
				if (ProcessingUnload.FindRef(ChunkPosition) == true)
				{
					GameManager->ChunkRegistry->UnregisterChunk(ChunkPosition);	
				}

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
			const auto Chunk = GameManager->ChunkRegistry->GetChunkActor(ChunkPosition);
			if (!Chunk)
			{
				continue;
			}

			const auto RenderId = LastRenderIndex++;
			
			auto& ProcessingRef = ProcessingRender.FindOrAdd(ChunkPosition);
			ProcessingRef.LastRenderIndex = RenderId;
			ProcessingRef.PendingTasks++;
			
			ToRemove.Add(ChunkPosition);

			const auto State = GameManager->VirtualMap->VirtualChunks.FindRef(ChunkPosition).State;
			Chunk->SetRenderState(State);

			GameManager->TickManager->RunAsyncThen(
				[Chunk]
				{
					FRenderResult Result;
					Result.bSuccess = Chunk->Th_BeginRender(Result.Mesh);
					return MoveTemp(Result);
				},
				[Chunk, this, ChunkPosition, RenderId](FRenderResult&& Result)
				{
					auto Processing = ProcessingRender.Find(ChunkPosition);
					Processing->PendingTasks--;
					// TODO analyze if Result.bSuccess may cause a mismatch? -> triggered one render, changed the RenderedAtChanges, but is not the last, so it's never committed
					if (Processing->LastRenderIndex == RenderId && Result.bSuccess)
					{
						Chunk->CommitRender(MoveTemp(Result.Mesh));
					}

					if (Processing->PendingTasks == 0) {
						ProcessingRender.Remove(ChunkPosition);
					}
				}
			);
		}

		for (int32 i = 0; i < ToRemove.Num(); ++i)
		{
			PendingRender.Remove(ToRemove[i]);
		}
	}
}
