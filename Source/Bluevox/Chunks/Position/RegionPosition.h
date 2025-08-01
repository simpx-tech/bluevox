#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/GameRules.h"
#include "RegionPosition.generated.h"

struct FChunkPosition;

USTRUCT(BlueprintType)
struct FRegionPosition
{
	GENERATED_BODY()

	FRegionPosition()
	{
	}

	static FRegionPosition FromChunkPosition(const FChunkPosition& ChunkPosition);

	UPROPERTY()
	int32 X = 0;

	UPROPERTY()
	int32 Y = 0;
};
