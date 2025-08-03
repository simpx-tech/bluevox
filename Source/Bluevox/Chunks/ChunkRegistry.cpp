// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkRegistry.h"

#include "RegionFile.h"
#include "Bluevox/Game/GameManager.h"
#include "Data/ChunkData.h"
#include "Position/LocalChunkPosition.h"

UChunkRegistry* UChunkRegistry::Init(const AGameManager* InGameManager)
{
	WorldSave = InGameManager->WorldSave;
	return this;
}

TSharedPtr<FRegionFile> UChunkRegistry::LoadRegionFile(const FRegionPosition& Position)
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

UChunkData* UChunkRegistry::GetChunkData(const FChunkPosition& Position)
{
	FScopeLock Lock(&ChunksDataLock);
	return ChunksData.FindRef(Position);
}

UChunkData* UChunkRegistry::FetchChunkDataFromDisk(const FChunkPosition& Position)
{
	const auto RegionPosition = FRegionPosition::FromChunkPosition(Position);
	const auto RegionFile = LoadRegionFile(RegionPosition);

	const auto ChunkData = NewObject<UChunkData>();
	RegionFile->LoadChunk(FLocalChunkPosition::FromChunkPosition(Position), ChunkData);

	return ChunkData;
}

bool UChunkRegistry::HasChunkData(const FChunkPosition& Position)
{
	FScopeLock Lock(&ChunksDataLock);
	return ChunksData.Contains(Position);
}

UChunkData* UChunkRegistry::LoadChunkData(const FChunkPosition& Position)
{
	{
		FScopeLock Lock(&ChunksDataLock);
		const auto ChunkData = GetChunkData(Position);
		if (ChunkData)
		{
			return ChunkData;
		}
	}
	
	const auto NewChunkData = FetchChunkDataFromDisk(Position);
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
