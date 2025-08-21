// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkTaskManager.h"

#include "LogVirtualMapTaskManager.h"
#include "VirtualMap.h"
#include "Bluevox/Chunk/Chunk.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/RegionFile.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/LocalChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Network/ChunkDataNetworkPacket.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Tick/TickManager.h"

void UChunkTaskManager::Sv_ProcessPendingNetSend(const FPendingNetSendChunks& PendingNetSend) const
{
	UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Processing pending net send for player %s, sending %d chunks"),
		*PendingNetSend.Player->GetName(), PendingNetSend.ToSend.Num());
	
	TArray<FChunkDataWithPosition> DataToSend;
	DataToSend.Reserve(PendingNetSend.ToSend.Num());
	for (const auto& ChunkPosition : PendingNetSend.ToSend)
	{
		if (!GameManager->ChunkRegistry->Th_HasChunkData(ChunkPosition))
		{
			UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Trying to send chunk %s to player %s, but it does not have data!"),
				*ChunkPosition.ToString(), *PendingNetSend.Player->GetName());
			continue;
		}

		DataToSend.Add(FChunkDataWithPosition{
			ChunkPosition,
			GameManager->ChunkRegistry->Th_GetChunkData(ChunkPosition)->Columns
		});
	}

	const auto Packet = NewObject<UChunkDataNetworkPacket>()->Init(MoveTemp(DataToSend));
	PendingNetSend.Player->PlayerNetwork->SendToClient(Packet);
}

UChunkTaskManager* UChunkTaskManager::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	bServer = InGameManager->bServer;
	return this;
}

void UChunkTaskManager::HandleChunkDataNetworkPacket(UChunkDataNetworkPacket* Packet)
{
	const auto ChunkRegistry = GameManager->ChunkRegistry;
	UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Handling chunk data network packet with %d chunks"), Packet->Data.Num());
	const auto TickManager = GameManager->TickManager;
	for (auto& ChunkData : Packet->Data)
	{
		const auto ChunkPosition = ChunkData.Position;

		if (!WaitingToBeSent.Contains(ChunkPosition))
		{
			// TODO temp
			// UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Received chunk data for chunk %s, but it was not in the waiting list!"), *ChunkPosition.ToString());
			// continue;
		}

		WaitingToBeSent.Remove(ChunkPosition);
		
		if (ChunkRegistry->Th_HasChunkData(ChunkPosition))
		{
			UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Received chunk data for chunk %s, but it already exists!"), *ChunkPosition.ToString());
			continue;
		}

		if (ProcessingUnload.Contains(ChunkPosition))
		{
			UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Received chunk data for chunk %s, but it is being unloaded! Ignoring the data."), *ChunkPosition.ToString());
			continue;
		}

		auto* ChunkDataObject = NewObject<UChunkData>(ChunkRegistry);
		ChunkDataObject->Init(GameManager, ChunkPosition, MoveTemp(ChunkData.Columns));
		ChunkRegistry->Th_RegisterChunk(ChunkPosition, ChunkDataObject);

		// Prevent stuttering the game
		TickManager->Th_ScheduleFn([this, ChunkPosition, ChunkRegistry]
		{
			if (!ProcessingUnload.Contains(ChunkPosition))
			{
				ChunkRegistry->SpawnChunk(ChunkPosition);	
			}
		});
	}
}

void UChunkTaskManager::ScheduleLoad(const TSet<FChunkPosition>& ChunksToLoad)
{
	if (!bServer)
	{
		UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Adding %d chunks to wait queue, waiting them to be sent from the server"), ChunksToLoad.Num());
		// DEV only add if not already exists
		WaitingToBeSent.Append(ChunksToLoad);
		return;
	}
	
	for (const auto& ChunkPosition : ChunksToLoad)
	{
		UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Scheduling load for chunk %s"), *ChunkPosition.ToString());
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
				UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Processing load for chunk %s"), *ChunkPosition.ToString());

				if (ProcessingLoad.FindRef(ChunkPosition) == true)
				{
					const auto ChunkData = NewObject<UChunkData>(GameManager->ChunkRegistry)->Init(GameManager, ChunkPosition,MoveTemp(Result.Columns));
					GameManager->ChunkRegistry->Th_RegisterChunk(ChunkPosition, ChunkData);

					if (PendingPacketsByPosition.Contains(ChunkPosition))
					{
						UE_LOG(LogVirtualMapTaskManager, VeryVerbose, TEXT("Chunk %s has PendingNetSend"), *ChunkPosition.ToString());
						TArray<int32> ToRemove;
						for (const auto& PackageIndex : PendingPacketsByPosition[ChunkPosition])
						{
							auto& PendingPacket = PendingPackets[PackageIndex];
							PendingPacket.WaitingFor.Remove(ChunkPosition);
							
							if (PendingPacket.WaitingFor.Num() == 0)
							{
								Sv_ProcessPendingNetSend(PendingPacket);
								PendingPackets.RemoveAt(PackageIndex);
							}
						}

						PendingPacketsByPosition.Remove(ChunkPosition);
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

				// We canceled the load, but it's automatically added to the ChunkRegistry, so we have to undo this
				if (ProcessingLoad.FindRef(ChunkPosition) == false)
				{
					GameManager->ChunkRegistry->Th_UnregisterChunk(ChunkPosition);
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

void UChunkTaskManager::ScheduleRender(const TSet<FChunkPosition>& ChunksToRender)
{
	for (const auto& ChunkPosition : ChunksToRender)
	{
		// Only add the ones that are not being unloaded
		if (ProcessingUnload.FindRef(ChunkPosition) == false)
		{
			// Not render here, only render when all chunks are loaded
			// TODO probably there's a better way to handle this
			PendingRender.Add(ChunkPosition);
		}
	}
}

void UChunkTaskManager::Sv_ScheduleNetSend(const AMainController* ToPlayer,
	const TSet<FChunkPosition>& ChunksToSend)
{
	if (ChunksToSend.Num() == 0)
	{
		return;
	}
	
	if (!bServer)
	{
		UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Trying to schedule a NetSend from client, ignoring it."));
		return;
	}
	
	const auto NewPendingNetSendIndex = PendingPackets.Add(FPendingNetSendChunks{});
	FPendingNetSendChunks& PendingNetSend = PendingPackets[NewPendingNetSendIndex];
	PendingNetSend.WaitingFor = ChunksToSend;
	PendingNetSend.Player = ToPlayer;
	PendingNetSend.ToSend = ChunksToSend.Array();
	
	for (const auto& ChunkPosition : ChunksToSend)
	{
		if (GameManager->ChunkRegistry->Th_HasChunkData(ChunkPosition))
		{
			PendingNetSend.WaitingFor.Remove(ChunkPosition);
		} else
		{
#if !UE_BUILD_SHIPPING
			if (!ProcessingLoad.Contains(ChunkPosition))
			{
				UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Chunk %s was requested for a NetSend, but it was not scheduled for load. Player: %s"),
					*ChunkPosition.ToString(), *ToPlayer->GetName());
			}
#endif

			if (ProcessingUnload.Contains(ChunkPosition))
			{
				UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Chunk %s was requested for a NetSend, but it was scheduled for unload, discarding it. Player: %s"),
					*ChunkPosition.ToString(), *ToPlayer->GetName());
			} else
			{
				PendingPacketsByPosition.FindOrAdd(ChunkPosition).Add(NewPendingNetSendIndex);
			}
		}
	}

	if (PendingNetSend.WaitingFor.Num() == 0)
	{
		Sv_ProcessPendingNetSend(PendingNetSend);
		PendingPackets.RemoveAt(NewPendingNetSendIndex);
	}
}

void UChunkTaskManager::ScheduleUnload(const TSet<FChunkPosition>& ChunksToUnload)
{
	if (!bServer)
	{
		for (const auto& ChunkPosition : ChunksToUnload)
		{
			WaitingToBeSent.Remove(ChunkPosition);
			GameManager->ChunkRegistry->Th_UnregisterChunk(ChunkPosition);

			if (ProcessingRender.Contains(ChunkPosition))
			{
				ProcessingRender.Find(ChunkPosition)->LastRenderIndex = -1;
			}
		}
		
		return;
	}
	
	for (const auto& ChunkPosition : ChunksToUnload)
	{
		if (ProcessingLoad.Contains(ChunkPosition))
		{
			UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Cancelling load for chunk %s"), *ChunkPosition.ToString());
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
			UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Failed to unload chunk %s: region %s file not found."), *ChunkPosition.ToString(), *RegionPosition.ToString());
			continue;
		}

		const auto ChunkData = GameManager->ChunkRegistry->Th_GetChunkData(ChunkPosition);
		GameManager->TickManager->RunAsyncThen(
			[this, LocalChunkPosition, ChunkData, Region]
			{
				if (!ChunkData)
				{
					UE_LOG(LogVirtualMapTaskManager, Warning, TEXT("Failed to unload chunk data at local position %s: chunk data not found."), *LocalChunkPosition.ToString());
					return;
				}
				Region->Th_SaveChunk(LocalChunkPosition, ChunkData);
			},
			[ChunkPosition, this]
			{
				UE_LOG(LogVirtualMapTaskManager, Verbose, TEXT("Processing unload for chunk %s"), *ChunkPosition.ToString());
				
				if (ProcessingUnload.FindRef(ChunkPosition) == true)
				{
					GameManager->ChunkRegistry->Th_UnregisterChunk(ChunkPosition);	
				}

				ProcessingUnload.Remove(ChunkPosition);
			}
		);
	}
}

TStatId UChunkTaskManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMapTaskManager, STATGROUP_Tickables);
}

void UChunkTaskManager::Tick(float DeltaTime)
{
	if (ProcessingLoad.Num() == 0 && WaitingToBeSent.Num() == 0 && PendingRender.Num() != 0)
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
			
			// If state = none = border, so we can't render it (as it will have no neighbors)
			if (State != EChunkState::None)
			{
				GameManager->ChunkRegistry->LockForRender(ChunkPosition);

				GameManager->TickManager->RunAsyncThen(
					[Chunk]
					{
						FRenderResult Result;
						Result.bSuccess = Chunk->BeginRender(Result.Mesh, Result.WaterMesh);
						return MoveTemp(Result);
					},
					[Chunk, this, ChunkPosition, RenderId](FRenderResult&& Result)
					{
						const auto Processing = ProcessingRender.Find(ChunkPosition);
						Processing->PendingTasks--;
						// TODO analyze if Result.bSuccess may cause a mismatch? -> triggered one render, changed the RenderedAtChanges, but is not the last, so it's never committed
						if (Processing->LastRenderIndex == RenderId && Result.bSuccess && Chunk)
						{
							Chunk->CommitRender(MoveTemp(Result));
						}

						if (Processing->PendingTasks == 0) {
							OnAllRenderTasksFinishedForChunk.Broadcast(ChunkPosition);
							ProcessingRender.Remove(ChunkPosition);
						}

						GameManager->ChunkRegistry->UnlockForRender(ChunkPosition);
					}
				);
			}
		}

		for (int32 i = 0; i < ToRemove.Num(); ++i)
		{
			PendingRender.Remove(ToRemove[i]);
		}
	}
}
