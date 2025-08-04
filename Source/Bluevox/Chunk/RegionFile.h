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

	UChunkData* Th_LoadChunk(const FLocalChunkPosition& Position);

	static TSharedPtr<FRegionFile> NewFromDisk(const FString& WorldName, const FRegionPosition& RegionPosition);
};
