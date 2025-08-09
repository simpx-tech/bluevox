// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMap.h"

#include "VirtualMapStats.h"
#include "VirtualMapTaskManager.h"
#include "Bluevox/Chunk/ChunkHelper.h"
#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"

void UVirtualMap::RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& ToRemoveLoad, const TSet<FChunkPosition>& ToRemoveLive)
{
	TSet<FChunkPosition> ToUnload;
	TSet<FChunkPosition> ToRender;

	const auto IsLocalPlayer = GameManager->LocalController == Controller;
	for (const auto& LivePosition : ToRemoveLive)
	{
		if (VirtualChunks.Contains(LivePosition))
		{
			auto& VirtualChunk = VirtualChunks[LivePosition];
			if (IsLocalPlayer)
			{
				VirtualChunk.bLocal = false;
			}

			VirtualChunk.LiveForCount--;

			if (VirtualChunk.ShouldBeKeptAlive())
			{
				VirtualChunk.RecalculateState();
				ToRender.Add(LivePosition);
			} else
			{
				ToUnload.Add(LivePosition);
			}
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Trying to remove player %s from chunk %s, but chunk does not exist!"),
				*Controller->GetName(), *LivePosition.ToString());
		}
	}

	for (const auto& LoadPosition : ToRemoveLoad)
	{
		if (VirtualChunks.Contains(LoadPosition))
		{
			auto& VirtualChunk = VirtualChunks[LoadPosition];
			if (IsLocalPlayer)
			{
				VirtualChunk.bLocal = false;
			}

			VirtualChunk.LoadedForCount--;

			if (VirtualChunk.ShouldBeKeptAlive())
			{
				ToRender.Add(LoadPosition);
				VirtualChunk.RecalculateState();
			} else
			{
				ToUnload.Add(LoadPosition);
			}
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Trying to remove player %s from chunk %s, but chunk does not exist!"),
				*Controller->GetName(), *LoadPosition.ToString());
		}
	}
	
	TaskManager->ScheduleUnload(ToUnload);
}

void UVirtualMap::AddPlayerToChunks(const AMainController* Controller,
	const TSet<FChunkPosition>& ToLoad, const TSet<FChunkPosition>& ToLoadAndRender)
{
	TSet<FChunkPosition> ScheduleLoad;
	TSet<FChunkPosition> ScheduleRender;
	TSet<FChunkPosition> ScheduleSend;
	
	const auto IsLocalPlayer = GameManager->LocalController == Controller;
	for (const auto& ChunkPosition : ToLoad)
	{
		if (VirtualChunks.Contains(ChunkPosition))
		{
			auto& VirtualChunk = VirtualChunks[ChunkPosition];
			VirtualChunk.LoadedForCount++;
			VirtualChunk.bLocal = VirtualChunk.bLocal || IsLocalPlayer;
		} else
		{
			FVirtualChunk NewVirtualChunk;
			NewVirtualChunk.LoadedForCount = 1;
			NewVirtualChunk.State = EChunkState::None;
			NewVirtualChunk.bLocal = IsLocalPlayer;
			VirtualChunks.Add(ChunkPosition, NewVirtualChunk);
			ScheduleLoad.Add(ChunkPosition);
		}

		if (!IsLocalPlayer)
		{
			ScheduleSend.Add(ChunkPosition);
		}
	}

	for (const auto& ChunkPosition : ToLoadAndRender)
	{
		if (VirtualChunks.Contains(ChunkPosition))
		{
			auto& VirtualChunk = VirtualChunks[ChunkPosition];
			VirtualChunk.LiveForCount++;
			VirtualChunk.State = IsLocalPlayer ? EChunkState::Live : EChunkState::RemoteLive;
			VirtualChunk.bLocal = VirtualChunk.bLocal || IsLocalPlayer;
		} else
		{
			FVirtualChunk NewVirtualChunk;
			NewVirtualChunk.LiveForCount = 1;
			NewVirtualChunk.State = IsLocalPlayer ? EChunkState::Live : EChunkState::RemoteLive;
			NewVirtualChunk.bLocal = IsLocalPlayer;
			VirtualChunks.Add(ChunkPosition, NewVirtualChunk);
			ScheduleLoad.Add(ChunkPosition);
		}

		ScheduleRender.Add(ChunkPosition);

		if (!IsLocalPlayer)
		{
			ScheduleSend.Add(ChunkPosition);
		}
	}

	TaskManager->ScheduleLoad(ScheduleLoad);
	TaskManager->ScheduleRender(ScheduleRender);
	TaskManager->ScheduleNetSend(Controller, ScheduleSend);
}

void UVirtualMap::HandleStateUpdate(const TSet<FChunkPosition>& LoadToLive, const TSet<FChunkPosition>& LiveToLoad)
{
	for (const auto& Pos : LoadToLive)
	{
		if (VirtualChunks.Contains(Pos))
		{
			auto& VirtualChunk = VirtualChunks[Pos];
			VirtualChunk.LoadedForCount--;
			VirtualChunk.LiveForCount++;
			VirtualChunk.RecalculateState();
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Trying to update state for chunk %s to Live, but chunk does not exist!"),
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
			VirtualChunk.RecalculateState();
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Trying to update state for chunk %s to CollisionOnly, but chunk does not exist!"),
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

	const auto AddedLive = CurLive.Difference(OldLive);
	const auto AddedLoad = CurLoad.Difference(OldLoad);

	const auto LoadToLive = OldLoad.Intersect(CurLive);
	const auto LiveToLoad = OldLive.Intersect(CurLoad);

	const auto RemovedLoad = OldLoad.Difference(AllCur);
	const auto RemovedLive = OldLive.Difference(AllCur);

	UE_LOG(LogChunk, Verbose, TEXT("Player %s moved from %s to %s. Added Load: %d, Added Live: %d, LoadToLive: %d, LiveToLoad: %d, RemovedLoad: %d, RemovedLive: %d"),
		*Controller->GetName(), *OldPosition.ToString(), *NewPosition.ToString(), AddedLoad.Num(), AddedLive.Num(), LoadToLive.Num(), LiveToLoad.Num(), RemovedLoad.Num(), RemovedLive.Num());

	AddPlayerToChunks(Controller, AddedLoad, AddedLive);
	RemovePlayerFromChunks(Controller, RemovedLoad, RemovedLive);

	// Border chunks
	TaskManager->ScheduleRender(LoadToLive.Union(LiveToLoad));
}

UVirtualMap* UVirtualMap::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	TaskManager = NewObject<UVirtualMapTaskManager>(this, TEXT("VirtualMapTaskManager"));
	TaskManager->Init(InGameManager);
	return this;
}

void UVirtualMap::RegisterPlayer(const AMainController* Player)
{
	const auto GlobalPosition = FChunkPosition::FromGlobalPosition(Player->SavedGlobalPosition);
	ChunkPositionByPlayer.Add(Player, GlobalPosition);

	TSet<FChunkPosition> LoadChunks;
	TSet<FChunkPosition> LiveChunks;
	UChunkHelper::GetChunksAroundLoadAndLive(GlobalPosition, Player->GetFarDistance(), LoadChunks, LiveChunks);

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

void UVirtualMap::UpdateFarDistanceForPlayer(const AMainController* Player,
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
