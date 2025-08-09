// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualMap.h"

#include "VirtualMapStats.h"
#include "VirtualMapTaskManager.h"
#include "Bluevox/Chunk/ChunkHelper.h"
#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
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
			if (IsLocalController)
			{
				EnumRemoveFlags(CurrentChunk->State, EChunkState::LocalLive);
			} else
			{
				CurrentChunk->LiveForRemotePlayersAmount--;
			}

			CurrentChunk->State = CalculateState(*CurrentChunk);

			// RemoteVisualOnly = No Collision, No Rendered + No remote players
			if (!EnumHasAnyFlags(CurrentChunk->State, EChunkState::Collision | EChunkState::Rendered) && CurrentChunk->FarForRemotePlayersAmount == 0)
			{
				ToUnload.Add(LivePosition);
			} else
			{
				ToRender.Add(LivePosition);
			}
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Chunk %s not found in virtual map when trying to remove player."), *LivePosition.ToString());
		}
	}

	for (const auto& FarPosition : FarChunks)
	{
		const auto CurrentChunk = VirtualChunks.Find(FarPosition);
		if (CurrentChunk)
		{
			if (IsLocalController)
			{
				EnumRemoveFlags(CurrentChunk->State, EChunkState::LocalFar);
			} else
			{
				CurrentChunk->FarForRemotePlayersAmount--;
			}

			CurrentChunk->State = CalculateState(*CurrentChunk);

			// RemoteVisualOnly = No Collision, No Rendered + No remote players
			if (!EnumHasAnyFlags(CurrentChunk->State, EChunkState::Collision | EChunkState::Rendered) && CurrentChunk->FarForRemotePlayersAmount == 0)
			{
				ToUnload.Add(FarPosition);
			} else
			{
				ToRender.Add(FarPosition);
			}
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Chunk %s not found in virtual map when trying to remove player."), *FarPosition.ToString());
		}
	}
}

void UVirtualMap::AddPlayerToChunks(const AMainController* Controller, const TSet<FChunkPosition>& NewLiveChunks, const TSet<FChunkPosition>& NewFarChunks, const TSet<FChunkPosition>& FarToLive, const TSet<FChunkPosition>& LiveToFar)
{
	TSet<FChunkPosition> ToLoad;
	ToLoad.Reserve(NewFarChunks.Num());

	TSet<FChunkPosition> ToRender;
	ToRender.Reserve(NewLiveChunks.Num());

	TSet<FChunkPosition> ToSend;
	ToSend.Reserve(NewFarChunks.Num() + NewLiveChunks.Num());

	const auto IsLocalController = Controller == GameManager->LocalController;
	
	for (auto& LivePosition : NewLiveChunks)
	{
		const auto CurrentChunk = VirtualChunks.Find(LivePosition);

		if (CurrentChunk)
		{
			if (IsLocalController)
			{
				EnumAddFlags(CurrentChunk->State, EChunkState::LocalLive);
			} else
			{
				CurrentChunk->LiveForRemotePlayersAmount++;
			}

			CurrentChunk->State = CalculateState(*CurrentChunk);
		} else
		{
			FVirtualChunk NewChunk;
			NewChunk.State = IsLocalController ? EChunkState::Live | EChunkState::LocalLive : EChunkState::RemoteLive;
			NewChunk.LiveForRemotePlayersAmount = IsLocalController ? 0 : 1;

			VirtualChunks.Add(LivePosition, NewChunk);
			ToLoad.Add(LivePosition);
		}

		ToRender.Add(LivePosition);

		if (!IsLocalController)
		{
			ToSend.Add(LivePosition);
		}
	}

	const auto PlayerChunkPosition = FChunkPosition::FromActorLocation(Controller->GetPawn()->GetActorLocation());
	const auto MaxDistance = Controller->GetFarDistance() - 1;
	for (auto& FarPosition : NewFarChunks)
	{
		const auto Distance = UChunkHelper::GetDistance(PlayerChunkPosition, FarPosition);
		const auto ShouldRender = Distance <= MaxDistance;
		
		// Already loaded
		if (VirtualChunks.Contains(FarPosition))
		{
			// Only affect rendering
			if (IsLocalController)
			{
				if (ShouldRender)
				{
					const auto CurrentChunk = VirtualChunks.Find(FarPosition);
					// Enable rendered, doesn't change the rest
					CurrentChunk->State = CurrentChunk->State | EChunkState::Rendered;
					ToRender.Add(FarPosition);
				}
			} else
			{
				ToSend.Add(FarPosition);
			}
		} else
		{
			FVirtualChunk NewChunk;
			NewChunk.State = IsLocalController
				? EChunkState::LocalFar | (ShouldRender ? EChunkState::Rendered : EChunkState::None)
				: EChunkState::RemoteVisualOnly;
			NewChunk.FarForRemotePlayersAmount = IsLocalController ? 0 : 1;
			VirtualChunks.Add(FarPosition, NewChunk);
			ToLoad.Add(FarPosition);

			if (IsLocalController)
			{
				if (ShouldRender)
				{
					ToRender.Add(FarPosition);
				}
			} else
			{
				ToSend.Add(FarPosition);
			}
		}
	}

	for (const auto& LivePosition : FarToLive)
	{
		const auto CurrentChunk = VirtualChunks.Find(LivePosition);
		if (CurrentChunk)
		{
			if (IsLocalController)
			{
				EnumRemoveFlags(CurrentChunk->State, EChunkState::LocalFar);
				EnumAddFlags(CurrentChunk->State, EChunkState::LocalLive);
			} else
			{
				CurrentChunk->FarForRemotePlayersAmount--;
				CurrentChunk->LiveForRemotePlayersAmount++;
			}

			CurrentChunk->State = CalculateState(*CurrentChunk);
			ToRender.Add(LivePosition);
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Chunk %s not found in virtual map when trying to move from Far to Live."), *LivePosition.ToString());
		}
	}

	for (const auto& FarPosition : LiveToFar)
	{
		const auto CurrentChunk = VirtualChunks.Find(FarPosition);
		if (CurrentChunk)
		{
			if (IsLocalController)
			{
				EnumRemoveFlags(CurrentChunk->State, EChunkState::LocalLive);
				EnumAddFlags(CurrentChunk->State, EChunkState::LocalFar);
			} else
			{
				CurrentChunk->LiveForRemotePlayersAmount--;
				CurrentChunk->FarForRemotePlayersAmount++;
			}

			CurrentChunk->State = CalculateState(*CurrentChunk);
			ToRender.Add(FarPosition);
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("Chunk %s not found in virtual map when trying to move from Live to Far."), *FarPosition.ToString());
		}
	}
	
	TaskManager->ScheduleLoad(ToLoad);
	TaskManager->ScheduleRender(ToRender);
	TaskManager->ScheduleNetSend(Controller, ToSend);
}

void UVirtualMap::HandlePlayerMovement(const AMainController* Controller,
	const FChunkPosition& OldPosition, const FChunkPosition& NewPosition)
{
	SCOPE_CYCLE_COUNTER(STAT_VirtualMap_HandlePlayerMovement);
	
	TSet<FChunkPosition> OldLive;
	TSet<FChunkPosition> OldFar;

	TSet<FChunkPosition> CurLive;
	TSet<FChunkPosition> CurFar;

	TSet<FChunkPosition> BorderChunks;
	
	UChunkHelper::GetChunksAroundLiveAndFar(OldPosition, Controller->GetFarDistance(), GameRules::Distances::LiveDistance, OldFar, OldLive);
	UChunkHelper::GetChunksAroundLiveAndFar(NewPosition, Controller->GetFarDistance(), GameRules::Distances::LiveDistance, CurFar, CurLive);
	UChunkHelper::GetBorderChunks(OldPosition, Controller->GetFarDistance(), BorderChunks);

	const auto AllOld = OldLive.Union(OldFar);
	const auto AllCur = CurLive.Union(CurFar);
	
	const auto FarAddedOnly = CurFar.Difference(AllOld);
	const auto LiveAddedOnly = CurLive.Difference(AllOld);

	const auto FarRemovedOnly = OldFar.Difference(AllCur);
	const auto LiveRemovedOnly = OldLive.Difference(AllCur);

	const auto FarToLive = OldFar.Intersect(CurLive);
	const auto LiveToFar = OldLive.Intersect(CurFar);

	const auto ToRenderBorder = BorderChunks.Intersect(AllCur);

	UE_LOG(LogChunk, Verbose, TEXT("Player %s moved from %s to %s. Added Live: %d, Added Far: %d, Removed Live: %d, Removed Far: %d"),
		*Controller->GetName(), *OldPosition.ToString(), *NewPosition.ToString(),
		LiveAddedOnly.Num(), FarAddedOnly.Num(), LiveRemovedOnly.Num(), FarRemovedOnly.Num());
	
	AddPlayerToChunks(Controller, LiveAddedOnly, FarAddedOnly, FarToLive, LiveToFar);
	RemovePlayerFromChunks(Controller, LiveRemovedOnly, FarRemovedOnly);
	TaskManager->ScheduleRender(ToRenderBorder);
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

	TSet<FChunkPosition> FarChunks;
	TSet<FChunkPosition> LiveChunks;
	UChunkHelper::GetChunksAroundLiveAndFar(GlobalPosition, Player->GetFarDistance(), GameRules::Distances::LiveDistance, FarChunks, LiveChunks);

	AddPlayerToChunks(Player, LiveChunks, FarChunks, {}, {});
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
			HandlePlayerMovement(Controller, LastPosition, CurrentChunkPosition);
			ChunkPositionByPlayer[Controller] = CurrentChunkPosition;
		}
	}
}

TStatId UVirtualMap::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVirtualMap, STATGROUP_Tickables);
}
