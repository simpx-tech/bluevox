// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMap.h"

#include "LogVirtualMap.h"
#include "VirtualMapStats.h"
#include "VirtualMapTaskManager.h"
#include "Bluevox/Chunk/ChunkHelper.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"

void UVirtualMap::RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& ToRemoveLoad, const TSet<FChunkPosition>& ToRemoveLive)
{
	UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Removing player %s from chunks. Load: %d, Live: %d"), 
		*Controller->GetName(), ToRemoveLoad.Num(), ToRemoveLive.Num());
	TSet<FChunkPosition> ToUnload;
	TSet<FChunkPosition> ToRender;

	const auto IsLocalPlayer = GameManager->LocalController == Controller;
	for (const auto& LivePosition : ToRemoveLive)
	{
		UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Removing player %s from Live chunk %s"), *Controller->GetName(), *LivePosition.ToString());
		if (VirtualChunks.Contains(LivePosition))
		{
			auto& VirtualChunk = VirtualChunks[LivePosition];
			if (IsLocalPlayer)
			{
				VirtualChunk.bLiveLocal = false;
			}

			VirtualChunk.LiveForCount--;

			if (VirtualChunk.ShouldBeKeptAlive())
			{
				VirtualChunk.RecalculateState();
				ToRender.Add(LivePosition);
			} else
			{
				VirtualChunks.Remove(LivePosition);
				ToUnload.Add(LivePosition);
			}

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("After remove player %s from Live chunk %s: State: %s, LiveFor: %d, LoadedFor: %d, bLocal: %d"), *Controller->GetName(), *LivePosition.ToString(), 
				*UEnum::GetValueAsString(VirtualChunk.State), VirtualChunk.LiveForCount, VirtualChunk.LoadedForCount, VirtualChunk.bLiveLocal);
		} else
		{
			UE_LOG(LogVirtualMap, Warning, TEXT("Trying to remove player %s from chunk %s, but chunk does not exist!"),
				*Controller->GetName(), *LivePosition.ToString());
		}
	}

	for (const auto& LoadPosition : ToRemoveLoad)
	{
		UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Removing player %s from Load chunk %s"), *Controller->GetName(), *LoadPosition.ToString());
		
		if (VirtualChunks.Contains(LoadPosition))
		{
			auto& VirtualChunk = VirtualChunks[LoadPosition];

			VirtualChunk.LoadedForCount--;

			if (VirtualChunk.ShouldBeKeptAlive())
			{
				VirtualChunk.RecalculateState();
				ToRender.Add(LoadPosition);
			} else
			{
				VirtualChunks.Remove(LoadPosition);
				ToUnload.Add(LoadPosition);
			}

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("After remove player %s from Load chunk %s: State: %s, LiveFor: %d, LoadedFor: %d, bLocal: %d"), *Controller->GetName(), *LoadPosition.ToString(),
				*UEnum::GetValueAsString(VirtualChunk.State), VirtualChunk.LiveForCount, VirtualChunk.LoadedForCount, VirtualChunk.bLiveLocal);
		} else
		{
			UE_LOG(LogVirtualMap, Warning, TEXT("Trying to remove player %s from chunk %s, but chunk does not exist!"),
				*Controller->GetName(), *LoadPosition.ToString());
		}
	}
	
	TaskManager->ScheduleUnload(ToUnload);
}

void UVirtualMap::AddPlayerToChunks(const AMainController* Controller,
	const TSet<FChunkPosition>& ToLoad, const TSet<FChunkPosition>& ToLive)
{
	TSet<FChunkPosition> ScheduleLoad;
	TSet<FChunkPosition> ScheduleRender;
	TSet<FChunkPosition> ScheduleSend;
	
	const auto IsLocalPlayer = GameManager->LocalController == Controller;
	for (const auto& ChunkPosition : ToLoad)
	{
		UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Adding player %s to Load chunk %s"), *Controller->GetName(), *ChunkPosition.ToString());
		
		if (VirtualChunks.Contains(ChunkPosition))
		{
			auto& VirtualChunk = VirtualChunks[ChunkPosition];
			VirtualChunk.LoadedForCount++;

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("After add player %s to Load chunk %s: State: %s, LiveFor: %d, LoadedFor: %d"), *Controller->GetName(), *ChunkPosition.ToString(),
				*UEnum::GetValueAsString(VirtualChunk.State), VirtualChunk.LiveForCount, VirtualChunk.LoadedForCount);
		} else
		{
			FVirtualChunk NewVirtualChunk;
			NewVirtualChunk.LoadedForCount = 1;
			NewVirtualChunk.State = EChunkState::None;
			NewVirtualChunk.bLiveLocal = false;
			VirtualChunks.Add(ChunkPosition, NewVirtualChunk);
			ScheduleLoad.Add(ChunkPosition);

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Added new Load chunk %s for player %s, State: %s, LiveFor: %d, LoadedFor: %d, bLocal: %d"),
				*ChunkPosition.ToString(), *Controller->GetName(),
				*UEnum::GetValueAsString(NewVirtualChunk.State), NewVirtualChunk.LiveForCount, NewVirtualChunk.LoadedForCount, NewVirtualChunk.bLiveLocal);
		}

		if (!IsLocalPlayer && bServer)
		{
			ScheduleSend.Add(ChunkPosition);
		}
	}

	for (const auto& ChunkPosition : ToLive)
	{
		UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Adding player %s to Live chunk %s"), *Controller->GetName(), *ChunkPosition.ToString());
		
		if (VirtualChunks.Contains(ChunkPosition))
		{
			auto& VirtualChunk = VirtualChunks[ChunkPosition];
			VirtualChunk.LiveForCount++;

			if (IsLocalPlayer)
			{
				VirtualChunk.bLiveLocal = true;
			}
			
			VirtualChunk.RecalculateState();

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("After add player %s to Live chunk %s: State: %s, LiveFor: %d, LoadedFor: %d, bLocal: %d"), *Controller->GetName(), *ChunkPosition.ToString(),
				*UEnum::GetValueAsString(VirtualChunk.State), VirtualChunk.LiveForCount, VirtualChunk.LoadedForCount, VirtualChunk.bLiveLocal);
		} else
		{
			FVirtualChunk NewVirtualChunk;
			NewVirtualChunk.LiveForCount = 1;
			NewVirtualChunk.State = IsLocalPlayer ? EChunkState::Live : EChunkState::RemoteLive;
			NewVirtualChunk.bLiveLocal = IsLocalPlayer;
			VirtualChunks.Add(ChunkPosition, NewVirtualChunk);
			ScheduleLoad.Add(ChunkPosition);

			UE_LOG(LogVirtualMap, VeryVerbose, TEXT("Added new Live chunk %s for player %s, State: %s, LiveFor: %d, LoadedFor: %d, bLocal: %d"),
				*ChunkPosition.ToString(), *Controller->GetName(),
				*UEnum::GetValueAsString(NewVirtualChunk.State), NewVirtualChunk.LiveForCount, NewVirtualChunk.LoadedForCount, NewVirtualChunk.bLiveLocal);
		}

		ScheduleRender.Add(ChunkPosition);

		if (!IsLocalPlayer && bServer)
		{
			ScheduleSend.Add(ChunkPosition);
		}
	}

	TaskManager->ScheduleLoad(ScheduleLoad);
	TaskManager->ScheduleRender(ScheduleRender);
	TaskManager->Sv_ScheduleNetSend(Controller, ScheduleSend);
}

void UVirtualMap::HandleStateUpdate(const AMainController* Controller, const TSet<FChunkPosition>& LoadToLive, const TSet<FChunkPosition>& LiveToLoad)
{
	const auto IsLocalPlayer = GameManager->LocalController == Controller;
	for (const auto& Pos : LoadToLive)
	{
		if (VirtualChunks.Contains(Pos))
		{
			auto& VirtualChunk = VirtualChunks[Pos];
			VirtualChunk.LoadedForCount--;
			VirtualChunk.LiveForCount++;
			if (IsLocalPlayer)
			{
				VirtualChunk.bLiveLocal = true;
			}
			VirtualChunk.RecalculateState();
		} else
		{
			UE_LOG(LogVirtualMap, Warning, TEXT("Trying to update state for chunk %s to Live, but chunk does not exist!"),
				*Pos.ToString());
		}
	}

	for (const auto& Pos : LiveToLoad)
	{
		if (VirtualChunks.Contains(Pos))
		{
			auto& VirtualChunk = VirtualChunks[Pos];
			VirtualChunk.LiveForCount--;
			VirtualChunk.LoadedForCount++;
			if (IsLocalPlayer)
			{
				VirtualChunk.bLiveLocal = false;
			}
			VirtualChunk.RecalculateState();
		} else
		{
			UE_LOG(LogVirtualMap, Warning, TEXT("Trying to update state for chunk %s to CollisionOnly, but chunk does not exist!"),
				*Pos.ToString());
		}
	}

	TaskManager->ScheduleRender(LoadToLive.Union(LiveToLoad));
}

void UVirtualMap::HandlePlayerMovement(const AMainController* Controller,
                                       const FChunkPosition& OldPosition, const FChunkPosition& NewPosition)
{
	SCOPE_CYCLE_COUNTER(STAT_VirtualMap_HandlePlayerMovement);
	
	TSet<FChunkPosition> OldLoad;
	TSet<FChunkPosition> OldLive;

	TSet<FChunkPosition> CurLoad;
	TSet<FChunkPosition> CurLive;
	
	UChunkHelper::GetChunksAroundLoadAndLive(OldPosition, Controller->GetFarDistance(), OldLoad, OldLive);
	UChunkHelper::GetChunksAroundLoadAndLive(NewPosition, Controller->GetFarDistance(), CurLoad, CurLive);

	const auto AllOld = OldLoad.Union(OldLive);
	const auto AllCur = CurLoad.Union(CurLive);

	const auto AddedLive = CurLive.Difference(AllOld);
	const auto AddedLoad = CurLoad.Difference(AllOld);

	const auto LoadToLive = OldLoad.Intersect(CurLive);
	const auto LiveToLoad = OldLive.Intersect(CurLoad);

	const auto RemovedLoad = OldLoad.Difference(AllCur);
	const auto RemovedLive = OldLive.Difference(AllCur);

	UE_LOG(LogVirtualMap, Verbose, TEXT("Player %s moved from %s to %s. Added Load: %d, Added Live: %d, LoadToLive: %d, LiveToLoad: %d, RemovedLoad: %d, RemovedLive: %d"),
		*Controller->GetName(), *OldPosition.ToString(), *NewPosition.ToString(), AddedLoad.Num(), AddedLive.Num(), LoadToLive.Num(), LiveToLoad.Num(), RemovedLoad.Num(), RemovedLive.Num());

	AddPlayerToChunks(Controller, AddedLoad, AddedLive);
	RemovePlayerFromChunks(Controller, RemovedLoad, RemovedLive);
	HandleStateUpdate(Controller, LoadToLive, LiveToLoad);
}

UVirtualMap* UVirtualMap::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	TaskManager = NewObject<UVirtualMapTaskManager>(this);
	TaskManager->Init(InGameManager);
	bServer = InGameManager->bServer;
	return this;
}

void UVirtualMap::RegisterPlayer(const AMainController* Player)
{
	const auto GlobalPosition = FChunkPosition::FromGlobalPosition(Player->SavedGlobalPosition);
	ChunkPositionByPlayer.Add(Player, GlobalPosition);

	TSet<FChunkPosition> LoadChunks;
	TSet<FChunkPosition> LiveChunks;
	UChunkHelper::GetChunksAroundLoadAndLive(GlobalPosition, Player->GetFarDistance(), LoadChunks, LiveChunks);

	UE_LOG(LogVirtualMap, Verbose, TEXT("Registering player %s at position %s with far distance %d"),
		*Player->GetName(), *GlobalPosition.ToString(), Player->GetFarDistance());
	
	AddPlayerToChunks(Player, LoadChunks, LiveChunks);
}

void UVirtualMap::UnregisterPlayer(const AMainController* Player)
{
	ChunkPositionByPlayer.Remove(Player);
	const auto GlobalPosition = FChunkPosition::FromGlobalPosition(Player->SavedGlobalPosition);

	TSet<FChunkPosition> LoadChunks;
	TSet<FChunkPosition> LiveChunks;
	UChunkHelper::GetChunksAroundLoadAndLive(GlobalPosition, Player->GetFarDistance(), LoadChunks, LiveChunks);

	RemovePlayerFromChunks(Player, LoadChunks, LiveChunks);
}

void UVirtualMap::Sv_UpdateFarDistanceForPlayer(const AMainController* Player,
	const int32 OldFarDistance, const int32 NewFarDistance)
{
	// DEV
}

void UVirtualMap::Tick(float DeltaTime)
{
	for (auto& [Controller, LastPosition] : ChunkPositionByPlayer)
	{
		const auto CurrentPosition = Controller->GetPawn()->GetActorLocation();
		const FChunkPosition CurrentChunkPosition = FChunkPosition::FromActorLocation(CurrentPosition);
		if (CurrentChunkPosition != LastPosition)
		{
			HandlePlayerMovement(Controller, LastPosition, CurrentChunkPosition);
			ChunkPositionByPlayer[Controller] = CurrentChunkPosition;
		}
	}
}

TStatId UVirtualMap::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMap, STATGROUP_Tickables);
}

ETickableTickType UVirtualMap::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Always;
}
