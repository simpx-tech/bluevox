// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkRegistry.h"

#include "Chunk.h"
#include "RegionFile.h"
#include "Bluevox/Game/GameManager.h"
#include "Data/ChunkData.h"
#include "Generator/WorldGenerator.h"
#include "Position/LocalChunkPosition.h"

UChunkRegistry* UChunkRegistry::Init(AGameManager* InGameManager)
{
	WorldSave = InGameManager->WorldSave;
	GameManager = InGameManager;
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

	const auto Chunk = GetWorld()->SpawnActor<AChunk>(
		AChunk::StaticClass(),
		FVector(
			Position.X * GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize,
			Position.Y * GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize,
			0.0f),
		FRotator::ZeroRotator);

	const auto ChunkData = Th_GetChunkData(Position);
	Chunk->Init(Position, GameManager, ChunkData);

	ChunkActors.Add(Position, Chunk);

	return Chunk;
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

void UChunkRegistry::LockForRender(const FChunkPosition& Position)
{
	Th_GetChunkData(Position)->Lock.ReadLock();
	Th_GetChunkData(Position + FChunkPosition{0, 1})->Lock.WriteLock();
	Th_GetChunkData(Position + FChunkPosition{0, -1})->Lock.WriteLock();
	Th_GetChunkData(Position + FChunkPosition{1, 0})->Lock.WriteLock();
	Th_GetChunkData(Position + FChunkPosition{-0, 0})->Lock.WriteLock();
}

void UChunkRegistry::ReleaseForRender(const FChunkPosition& Position)
{
	Th_GetChunkData(Position)->Lock.ReadUnlock();
	Th_GetChunkData(Position + FChunkPosition{0, 1})->Lock.WriteUnlock();
	Th_GetChunkData(Position + FChunkPosition{0, -1})->Lock.WriteUnlock();
	Th_GetChunkData(Position + FChunkPosition{1, 0})->Lock.WriteUnlock();
	Th_GetChunkData(Position + FChunkPosition{-0, 0})->Lock.WriteUnlock();
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

	return RegionFile->Th_LoadChunk(FLocalChunkPosition::FromChunkPosition(Position));
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

	const auto ChunkData = NewObject<UChunkData>(this, UChunkData::StaticClass());
	ChunkData->ClearInternalFlags(EInternalObjectFlags::Async);
	WorldSave->WorldGenerator->GenerateChunk(Position, ChunkData);

	return ChunkData;
}

AChunk* UChunkRegistry::GetChunkActor(const FChunkPosition& Position) const
{
	if (ChunkActors.Contains(Position))
	{
		return ChunkActors[Position];
	}

	return nullptr;
}
