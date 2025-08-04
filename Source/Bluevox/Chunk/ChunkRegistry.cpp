// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkRegistry.h"

#include "Chunk.h"
#include "RegionFile.h"
#include "Bluevox/Game/GameManager.h"
#include "Data/ChunkData.h"
#include "Position/LocalChunkPosition.h"

UChunkRegistry* UChunkRegistry::Init(const AGameManager* InGameManager)
{
	WorldSave = InGameManager->WorldSave;
	return this;
}

TSharedPtr<FRegionFile> UChunkRegistry::Th_LoadRegionFile(const FRegionPosition& Position)
{
	{
		FScopeLock Lock(&RegionsLock);
		if (Regions.Contains(Position))
		{
			return Regions[Position];
		}
	}

	const auto RegionFile = WorldSave->GetRegionFromDisk(Position);
	{
		FScopeLock Lock(&RegionsLock);
		Regions.Add(Position, RegionFile);
		return Regions[Position];
	}
}

TSharedPtr<FRegionFile> UChunkRegistry::Th_GetRegionFile(const FRegionPosition& Position)
{
	{
		FScopeLock Lock(&RegionsLock);
		if (Regions.Contains(Position))
		{
			return Regions[Position];
		}
	}

	return nullptr;
}

AChunk* UChunkRegistry::SpawnChunk(const FChunkPosition Position)
{
	if (ChunkActors.Contains(Position))
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk actor already exists for position %s"), *Position.ToString());
		return ChunkActors[Position];
	}

	const auto Actor = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), FVector(Position.X * GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize, Position.Y * GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize, 0.0f), FRotator::ZeroRotator);

	ChunkActors.Add(Position, Actor);

	return Actor;
}

void UChunkRegistry::RemoveChunk(const FChunkPosition& Position)
{
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
		FScopeLock Lock(&ChunksDataLock);
		if (ChunksData.Contains(Position))
		{
			ChunksData.Remove(Position);
		}
	}
}

UChunkData* UChunkRegistry::Th_GetChunkData(const FChunkPosition& Position)
{
	FScopeLock Lock(&ChunksDataLock);
	return ChunksData.FindRef(Position);
}

UChunkData* UChunkRegistry::Th_FetchChunkDataFromDisk(const FChunkPosition& Position)
{
	const auto RegionPosition = FRegionPosition::FromChunkPosition(Position);
	const auto RegionFile = Th_LoadRegionFile(RegionPosition);

	const auto ChunkData = NewObject<UChunkData>();
	RegionFile->Th_LoadChunk(FLocalChunkPosition::FromChunkPosition(Position), ChunkData);

	return ChunkData;
}

bool UChunkRegistry::Th_HasChunkData(const FChunkPosition& Position)
{
	FScopeLock Lock(&ChunksDataLock);
	return ChunksData.Contains(Position);
}

UChunkData* UChunkRegistry::Th_LoadChunkData(const FChunkPosition& Position)
{
	{
		FScopeLock Lock(&ChunksDataLock);
		const auto ChunkData = Th_GetChunkData(Position);
		if (ChunkData)
		{
			return ChunkData;
		}
	}
	
	const auto NewChunkData = Th_FetchChunkDataFromDisk(Position);
	if (NewChunkData)
	{
		{
			FScopeLock Lock(&ChunksDataLock);
			ChunksData.Add(Position, NewChunkData);
		}
		
		return NewChunkData;
	}

	return nullptr;
}

AChunk* UChunkRegistry::GetChunkActor(const FChunkPosition& Position) const
{
	if (ChunkActors.Contains(Position))
	{
		return ChunkActors[Position];
	}

	return nullptr;
}
