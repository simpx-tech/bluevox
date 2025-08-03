#pragma once

#include "CoreMinimal.h"
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

	bool operator==(FRegionPosition const& Other) const
	{
		return X == Other.X
			&& Y == Other.Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FRegionPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y));
}
