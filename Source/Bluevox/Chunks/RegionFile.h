#pragma once

#include "CoreMinimal.h"
#include "RegionFile.generated.h"

class UChunkData;

USTRUCT(BlueprintType)
struct FRegionFile
{
	GENERATED_BODY()

	FRegionFile()
	{
	}

	// DEV file handle

	void SaveChunk(FIntVector2 Position, UChunkData* ChunkData);
};
