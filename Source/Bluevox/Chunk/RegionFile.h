#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/WorldSave.h"
#include "Bluevox/Utils/SegmentedFile/SegmentedFile.h"
#include "Data/ChunkColumn.h"
#include "Bluevox/Entity/EntityTypes.h"

struct FLocalChunkPosition;
struct FLocalPosition;
class UChunkData;

struct FRegionFile : FSegmentedFile
{
	FRegionFile()
	{
	}

	void Th_SaveChunk(const FLocalChunkPosition& Position, UChunkData* ChunkData);

	bool Th_LoadChunk(const FLocalChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
	                  TArray<FEntityRecord>& OutEntities);

	static TSharedPtr<FRegionFile> NewFromDisk(const FString& WorldName, const FRegionPosition& RegionPosition);
};
