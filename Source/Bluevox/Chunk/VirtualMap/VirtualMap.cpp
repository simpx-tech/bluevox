// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMap.h"

#include "VirtualMapTaskManager.h"
#include "Bluevox/Chunk/ChunkHelper.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Chunk/Position/RegionPosition.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"

void UVirtualMap::RemovePlayerFromChunks(const AMainController* Controller, const TSet<FChunkPosition>& LiveChunks, const TSet<FChunkPosition>& FarChunks)
{
	TSet<FChunkPosition> ToRender;
	TSet<FChunkPosition> ToUnload;
	
	const auto IsLocalController = Controller == GameManager->LocalController;
	for (const auto& LivePosition : LiveChunks)
	{
		const auto CurrentChunk = VirtualChunks.Find(LivePosition);
		if (CurrentChunk)
		{
			const auto RegionPos = FRegionPosition::FromChunkPosition(LivePosition);
			ChunksByRegion.FindOrAdd(RegionPos)--;
			
			CurrentChunk->LiveForPlayersAmount--;

			// Downgrade live -> visual
			if (CurrentChunk->LiveForPlayersAmount <= 0)
			{
				CurrentChunk->State = IsLocalController ? EChunkState::VisualOnly : EChunkState::RemoteVisualOnly;
				ToRender.Add(LivePosition);

				if (!IsLocalController)
				{
					ToUnload.Add(LivePosition);
				}
			}
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Chunk %s not found in virtual map when trying to remove player."), *LivePosition.ToString());
		}
	}

	for (const auto& FarPosition : FarChunks)
	{
		const auto CurrentChunk = VirtualChunks.Find(FarPosition);
		if (CurrentChunk)
		{
			const auto RegionPos = FRegionPosition::FromChunkPosition(FarPosition);
			ChunksByRegion.FindOrAdd(RegionPos)--;
			
			if (CurrentChunk->LiveForPlayersAmount <= 0)
			{
				// Downgrade visual -> remote visual
				ToUnload.Add(FarPosition);
			} else
			{
				// Downgrade live -> remote live
				ToRender.Add(FarPosition);
			}
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Chunk %s not found in virtual map when trying to remove player."), *FarPosition.ToString());
		}
	}
}

void UVirtualMap::AddPlayerToChunks(const AMainController* Controller, const TSet<FChunkPosition>& LiveChunks, const TSet<FChunkPosition>& FarChunks)
{
	TSet<FChunkPosition> ToLoad;
	ToLoad.Reserve(FarChunks.Num());

	TSet<FChunkPosition> ToRender;
	ToRender.Reserve(LiveChunks.Num());

	TSet<FChunkPosition> ToUnload;
	ToUnload.Reserve(FarChunks.Num());

	TSet<FChunkPosition> ToSend;
	ToSend.Reserve(FarChunks.Num() + LiveChunks.Num());

	const auto IsLocalController = Controller == GameManager->LocalController;
	
	for (auto& LivePosition : LiveChunks)
	{
		const auto CurrentChunk = VirtualChunks.Find(LivePosition);

		if (CurrentChunk)
		{
			CurrentChunk->LiveForPlayersAmount++;
			CurrentChunk->State = EChunkState::Live;
		} else
		{
			FVirtualChunk NewChunk;
			NewChunk.State = EChunkState::Live;
			NewChunk.LiveForPlayersAmount = 1;
			VirtualChunks.Add(LivePosition, NewChunk);
		
			ToLoad.Add(LivePosition);

			const auto RegionPos = FRegionPosition::FromChunkPosition(LivePosition);
			ChunksByRegion.FindOrAdd(RegionPos)++;
		}

		ToRender.Add(LivePosition);

		if (!IsLocalController)
		{
			ToSend.Add(LivePosition);
		}
	}

	const auto PlayerChunkPosition = FChunkPosition::FromActorLocation(Controller->GetPawn()->GetActorLocation());
	const auto MaxDistance = Controller->GetFarDistance() - 1;
	for (auto& FarPosition : FarChunks)
	{
		const auto Distance = UChunkHelper::GetDistance(PlayerChunkPosition, FarPosition);
		
		// Already loaded
		if (VirtualChunks.Contains(FarPosition))
		{
			// Only affect rendering, if is local player, we should also render it visually
			if (IsLocalController)
			{
				const auto CurrentChunk = VirtualChunks.Find(FarPosition);
				// Enable rendered, doesn't change the collision state
				CurrentChunk->State = CurrentChunk->State | EChunkState::Rendered;

				if (Distance <= MaxDistance)
				{
					ToRender.Add(FarPosition);
				}
			} else
			{
				ToSend.Add(FarPosition);
			}
		} else
		{
			FVirtualChunk NewChunk;
			NewChunk.State = IsLocalController ? EChunkState::VisualOnly : EChunkState::RemoteVisualOnly;
			VirtualChunks.Add(FarPosition, NewChunk);
			ToLoad.Add(FarPosition);

			if (IsLocalController)
			{
				if (Distance <= MaxDistance)
				{
					ToRender.Add(FarPosition);
				}
			} else
			{
				ToSend.Add(FarPosition);
			}
			
			const auto RegionPos = FRegionPosition::FromChunkPosition(FarPosition);
			ChunksByRegion.FindOrAdd(RegionPos)++;
		}
	}
	
	TaskManager->ScheduleLoad(ToLoad);
	TaskManager->ScheduleRender(ToRender);
	TaskManager->ScheduleNetSend(Controller, ToSend);
}

void UVirtualMap::HandlePlayerMovement(const AMainController* Controller,
	const FChunkPosition& OldPosition, const FChunkPosition& NewPosition)
{
	TSet<FChunkPosition> OldLive;
	TSet<FChunkPosition> OldFar;

	TSet<FChunkPosition> CurLive;
	TSet<FChunkPosition> CurFar;
	
	UChunkHelper::GetChunksAroundLiveAndFar(OldPosition, Controller->GetFarDistance(), GameRules::Distances::LiveDistance, OldFar, OldLive);
	UChunkHelper::GetChunksAroundLiveAndFar(NewPosition, Controller->GetFarDistance(), GameRules::Distances::LiveDistance, CurFar, CurLive);

	const auto AllOld = OldLive.Union(OldFar);
	const auto AllCur = CurLive.Union(CurFar);
	
	const auto FarAddedOnly = CurFar.Difference(AllOld);
	const auto LiveAddedOnly = CurLive.Difference(AllOld);

	const auto FarRemovedOnly = OldFar.Difference(AllCur);
	const auto LiveRemovedOnly = OldLive.Difference(AllCur);

	const auto FarToLive = OldFar.Intersect(CurLive);
	const auto LiveToFar = OldLive.Intersect(CurFar);

	const auto AddedFar = FarAddedOnly.Union(LiveToFar);
	const auto AddedLive = LiveAddedOnly.Union(FarToLive);
	
	AddPlayerToChunks(Controller, AddedLive, AddedFar);
	RemovePlayerFromChunks(Controller, LiveRemovedOnly, FarRemovedOnly);
}

UVirtualMap* UVirtualMap::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	TaskManager = NewObject<UVirtualMapTaskManager>(this, TEXT("VirtualMapTaskManager"));
	// DEV temp
	TaskManager->AddToRoot();
	TaskManager->Init(InGameManager);
	return this;
}

void UVirtualMap::RegisterPlayer(const AMainController* Player)
{
	const auto GlobalPosition = FChunkPosition::FromGlobalPosition(Player->SavedGlobalPosition);
	ChunkPositionByPlayer.Add(Player, GlobalPosition);

	TSet<FChunkPosition> FarChunks;
	TSet<FChunkPosition> LiveChunks;
	UChunkHelper::GetChunksAroundLiveAndFar(GlobalPosition, Player->GetFarDistance(), GameRules::Distances::LiveDistance, FarChunks, LiveChunks);

	AddPlayerToChunks(Player, LiveChunks, FarChunks);
}

void UVirtualMap::UnregisterPlayer(const AMainController* Player)
{
	ChunkPositionByPlayer.Remove(Player);
	const auto GlobalPosition = FChunkPosition::FromGlobalPosition(Player->SavedGlobalPosition);

	TSet<FChunkPosition> LiveChunks;
	TSet<FChunkPosition> FarChunks;
	UChunkHelper::GetChunksAroundLiveAndFar(GlobalPosition, Player->GetFarDistance(), GameRules::Distances::LiveDistance, FarChunks, LiveChunks);

	RemovePlayerFromChunks(Player, LiveChunks, FarChunks);
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
			ChunkPositionByPlayer[Controller] = CurrentChunkPosition;
			HandlePlayerMovement(Controller, LastPosition, CurrentChunkPosition);
		}
	}
}

TStatId UVirtualMap::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMap, STATGROUP_Tickables);
}
