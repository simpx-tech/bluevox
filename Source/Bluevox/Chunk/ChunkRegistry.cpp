// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkRegistry.h"

#include "Chunk.h"
#include "LogChunk.h"
#include "RegionFile.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/Shape.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/Utils/Face.h"
#include "Data/ChunkData.h"
#include "Position/LocalChunkPosition.h"
#include "Position/LocalPosition.h"
#include "VirtualMap/ChunkTaskManager.h"

UChunkRegistry* UChunkRegistry::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	bServer = GameManager->bServer;
	return this;
}

TSharedPtr<FRegionFile> UChunkRegistry::Th_LoadRegionFile(const FRegionPosition& Position)
{
	FWriteScopeLock Lock(RegionsLock);
	if (Regions.Contains(Position))
	{
		return Regions[Position];
	}

	const auto RegionFile = GameManager->WorldSave->GetRegionFromDisk(Position);
	{
		Regions.Add(Position, RegionFile);
		return Regions[Position];
	}
}

TSharedPtr<FRegionFile> UChunkRegistry::Th_GetRegionFile(const FRegionPosition& Position)
{
	{
		FReadScopeLock Lock(RegionsLock);
		if (Regions.Contains(Position))
		{
			return Regions[Position];
		}
	}

	return nullptr;
}

AChunk* UChunkRegistry::SpawnChunk(const FChunkPosition Position)
{
	UE_LOG(LogChunk, Verbose, TEXT("Spawning chunk at position %s"), *Position.ToString());
	
	if (ChunkActors.Contains(Position))
	{
		UE_LOG(LogChunk, Warning, TEXT("Chunk actor already exists for position %s"), *Position.ToString());
		return ChunkActors[Position];
	}

	if (!Th_HasChunkData(Position))
	{
		UE_LOG(LogChunk, Error, TEXT("Trying to spawn chunk actor for position %s, but chunk data does not exist!"), *Position.ToString());
		return nullptr;
	}

	const auto Chunk = GetWorld()->SpawnActor<AChunk>(
		AChunk::StaticClass(),
		FVector(
			Position.X * GameConstants::Chunk::Size * GameConstants::Scaling::XYWorldSize,
			Position.Y * GameConstants::Chunk::Size * GameConstants::Scaling::XYWorldSize,
			0.0f),
		FRotator::ZeroRotator);

	if (!Chunk)
	{
		UE_LOG(LogChunk, Error, TEXT("Failed to spawn chunk actor for position %s"), *Position.ToString());
		return nullptr;
	}
	
	const auto ChunkData = Th_GetChunkData(Position);
	Chunk->Init(Position, GameManager, ChunkData);

	ChunkActors.Add(Position, Chunk);

	return Chunk;
}

FChunkColumn& UChunkRegistry::Th_GetColumn(const FColumnPosition& GlobalColPosition)
{
	FReadScopeLock Lock(ChunksDataLock);
	const FChunkPosition ChunkPos = FChunkPosition::FromColumnPosition(GlobalColPosition);
	const FLocalColumnPosition LocalColPosition = FLocalColumnPosition::FromColumnPosition(GlobalColPosition);

	checkf(ChunksData.Contains(ChunkPos),
		   TEXT("Chunk column not found for position %s"), *GlobalColPosition.ToString());

	return ChunksData[ChunkPos]->GetColumn(LocalColPosition);
}

void UChunkRegistry::SetPiece(const FGlobalPosition& GlobalPosition, FPiece&& InPiece)
{
	const auto ChunkPosition = FChunkPosition::FromGlobalPosition(GlobalPosition);
	const auto LocalPosition = FLocalPosition::FromGlobalPosition(GlobalPosition);

	const auto ChunkData = Th_GetChunkData(ChunkPosition);
	TArray<uint16> RemovedPiecesZ;
	TPair<TOptional<FChangeFromSet>, TOptional<FChangeFromSet>> ChangedPieces;
	ChunkData->Th_SetPiece(LocalPosition.X, LocalPosition.Y, LocalPosition.Z, MoveTemp(InPiece), RemovedPiecesZ, ChangedPieces);
	const auto ShapeRegistry = GameManager->ShapeRegistry;

	if (ShapeRegistry->GetShapeById(InPiece.Id)->ShouldTickOnPlace())
	{
		ChunkData->ScheduledToTick.Add(FLocalPosition::FromGlobalPosition(GlobalPosition));	
	}

	// DEV if it was already scheduled/all tick, we should update it too here (from changed)
	// DEV 2 cases -> both, add tick OR top -> move tick
	
	// Notify neighbors about the piece change
	for (const auto Face : FaceUtils::AllFaces)
	{
		const auto Offset = FaceUtils::GetOffsetByFace(Face);
		const auto Piece = ChunkData->Th_GetPieceCopy(LocalPosition.X + Offset.X, LocalPosition.Y + Offset.Y, LocalPosition.Z + Offset.Z);

		const auto Shape = ShapeRegistry->GetShapeById(Piece.Id);
		if (Shape)
		{
			if (Shape->ShouldTickOnNeighborUpdate())
			{
				const auto NeighborPosition = GlobalPosition + Offset;
				const auto ChunkPos = FChunkPosition::FromGlobalPosition(GlobalPosition + Offset);
				const auto Chunk = GetChunkActor(ChunkPos);

				if (Chunk)
				{
					Chunk->ChunkData->ScheduledToTick.Add(FLocalPosition::FromGlobalPosition(NeighborPosition));
				} else
				{
					UE_LOG(LogChunk, Warning, TEXT("SetPiece:FaceLoop: Chunk actor not found for position %s"), *ChunkPos.ToString());
				}
			}
		} else
		{
			UE_LOG(LogChunk, Warning, TEXT("SetPiece:FaceLoop: Shape not found for piece ID %d at position %s"), Piece.Id, *GlobalPosition.ToString());
		}
	}
	
	GameManager->ChunkTaskManager->ScheduleRender({ ChunkPosition });

	TArray<FChunkPosition> NeighborsToRender;
	GlobalPosition.GetBorderChunks(NeighborsToRender);
	if (NeighborsToRender.Num() > 0)
	{
		const TSet<FChunkPosition> UniqueNeighborsToRender = TSet(NeighborsToRender);
		GameManager->ChunkTaskManager->ScheduleRender(UniqueNeighborsToRender);
	}
}

void UChunkRegistry::SetPiece(const FGlobalPosition& GlobalPosition, const FPiece& Piece)
{
	auto PieceCopy = Piece;
	SetPiece(GlobalPosition, MoveTemp(PieceCopy));
}

void UChunkRegistry::Th_UnregisterChunk(const FChunkPosition& Position)
{
	UE_LOG(LogChunk, Verbose, TEXT("Unregistering chunk at position %s"), *Position.ToString());

	if (ChunksMarkedForUse.Contains(Position))
	{
		UE_LOG(LogChunk, Verbose, TEXT("Chunk %s is marked for use, scheduling removal later."), *Position.ToString());
		ChunksScheduledToRemove.Add(Position);
		return;
	}
	
	if (ChunkActors.Contains(Position))
	{
		const auto ChunkActor = ChunkActors[Position];
		if (ChunkActor)
		{
			ChunkActor->Destroy();
		}
		ChunkActors.Remove(Position);
	}

	{
		FWriteScopeLock Lock(ChunksDataLock);
		if (ChunksData.Contains(Position))
		{
			ChunksData.Remove(Position);
		}
	}

	const auto RegionPosition = FRegionPosition::FromChunkPosition(Position);
	{
		FWriteScopeLock Lock(RegionsLock);
		LoadedByRegion.FindOrAdd(RegionPosition) -= 1;
		if (LoadedByRegion[RegionPosition] <= 0)
		{
			UE_LOG(LogChunk, Verbose, TEXT("Unloading region %s because no chunks are loaded."), *RegionPosition.ToString());
			LoadedByRegion.Remove(RegionPosition);
			Regions.Remove(RegionPosition);
		}
	}
}

void UChunkRegistry::LockForRender(const FChunkPosition& Position)
{
	static const std::array Offsets = {
		FChunkPosition{0, 0},
		FChunkPosition{0, 1},
		FChunkPosition{0, -1},
		FChunkPosition{1, 0},
		FChunkPosition{-1, 0}
	};
	
	{
		FReadScopeLock Lock(ChunksDataLock);

		for (const auto& Offset : Offsets)
		{
			ChunksData.FindRef(Position + Offset)->Lock.ReadLock();
		}
	}

	{
		FWriteScopeLock WriteLock(ChunksDataLock);
		for (const auto& Offset : Offsets)
		{
			ChunksMarkedForUse.FindOrAdd(Position + Offset) += 1;
		}
	}
}

void UChunkRegistry::UnlockForRender(const FChunkPosition& Position)
{
	TArray<FChunkPosition> RemoveChunksMarkedForUse;

	{
		FReadScopeLock Lock(ChunksDataLock);
	
		static const std::array Offsets = {
			FChunkPosition{0, 0},
			FChunkPosition{0, 1},
			FChunkPosition{0, -1},
			FChunkPosition{1, 0},
			FChunkPosition{-1, 0}
		};
		
		for (const auto& Offset : Offsets)
		{
			ChunksData.FindRef(Position + Offset)->Lock.ReadUnlock();
			if (ChunksMarkedForUse.Contains(Position + Offset))
			{
				auto& AmountMarkedForUse = *ChunksMarkedForUse.Find(Position + Offset);
				AmountMarkedForUse -= 1;
				
				if (AmountMarkedForUse <= 0)
				{
					RemoveChunksMarkedForUse.Add(Position + Offset);
				}
			}
		}
	}

	for (const auto& ChunkPosition : RemoveChunksMarkedForUse)
	{
		UE_LOG(LogChunk, Verbose, TEXT("Chunk %s is no longer marked for use."), *ChunkPosition.ToString());
		ChunksMarkedForUse.Remove(ChunkPosition);
		if (ChunksScheduledToRemove.Contains(ChunkPosition))
		{
			UE_LOG(LogChunk, Verbose, TEXT("Chunk %s was scheduled for removal, removing now."), *ChunkPosition.ToString());
			Th_UnregisterChunk(ChunkPosition);
			ChunksScheduledToRemove.Remove(ChunkPosition);
		}
	}
}

UChunkData* UChunkRegistry::Th_GetChunkData(const FChunkPosition& Position)
{
	FReadScopeLock Lock(ChunksDataLock);
	return ChunksData.FindRef(Position);
}

void UChunkRegistry::Th_RegisterChunk(const FChunkPosition& Position, UChunkData* Data)
{
	UE_LOG(LogChunk, Verbose, TEXT("Registering chunk data for position %s"), *Position.ToString());
	FWriteScopeLock Lock(ChunksDataLock);

	if (!ChunksData.Contains(Position) && bServer)
	{
		LoadedByRegion.FindOrAdd(FRegionPosition::FromChunkPosition(Position)) += 1;
	}

	ChunksScheduledToRemove.Remove(Position);
	ChunksData.Add(Position, Data);
}

// TODO if used in other places, may cause to have unused RegionFiles
bool UChunkRegistry::Th_FetchChunkDataFromDisk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns)
{
	UE_LOG(LogChunk, Verbose, TEXT("Fetching chunk data from disk for position %s"), *Position.ToString());
	
	const auto RegionPosition = FRegionPosition::FromChunkPosition(Position);
	const auto RegionFile = Th_LoadRegionFile(RegionPosition);

	return RegionFile->Th_LoadChunk(FLocalChunkPosition::FromChunkPosition(Position), OutColumns);
}

bool UChunkRegistry::Th_HasChunkData(const FChunkPosition& Position)
{
	FReadScopeLock Lock(ChunksDataLock);
	return ChunksData.Contains(Position);
}

AChunk* UChunkRegistry::GetChunkActor(const FChunkPosition& Position) const
{
	if (ChunkActors.Contains(Position))
	{
		return ChunkActors[Position];
	}

	return nullptr;
}
