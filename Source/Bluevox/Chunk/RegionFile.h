#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/WorldSave.h"
#include "Bluevox/Utils/SegmentedFile/SegmentedFile.h"

struct FLocalChunkPosition;
struct FLocalPosition;
class UChunkData;

struct FRegionFile : FSegmentedFile
{
	FRegionFile()
	{
	}

	void Th_SaveChunk(const FLocalChunkPosition& Position, UChunkData* ChunkData);

	void Th_LoadChunk(const FLocalChunkPosition& Position, UChunkData* OutChunkDat);

	static TSharedPtr<FRegionFile> NewFromDisk(const FString& WorldName, const FRegionPosition& RegionPosition);
};
