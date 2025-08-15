#pragma once

#include "CoreMinimal.h"
#include "ChunkPosition.h"
#include "Bluevox/Utils/PositiveMod.h"
#include "LocalChunkPosition.generated.h"

USTRUCT(BlueprintType)
struct FLocalChunkPosition
{
	GENERATED_BODY()

	FLocalChunkPosition()
	{
	}

	FLocalChunkPosition(const uint8 InX, const uint8 InY)
		: X(InX), Y(InY)
	{
	}

	static FLocalChunkPosition FromChunkPosition(const FChunkPosition& ChunkPosition)
	{
		return {
			static_cast<uint8>(PositiveMod(ChunkPosition.X, GameConstants::Region::Size)),
			static_cast<uint8>(PositiveMod(ChunkPosition.X, GameConstants::Region::Size)),
		};
	}

	UPROPERTY()
	uint8 X = 0;

	UPROPERTY()
	uint8 Y = 0;

	FString ToString() const
	{
		return FString::Printf(TEXT("LocalChunkPosition(X: %d, Y: %d)"), X, Y);
	}
};

FORCEINLINE uint32 GetTypeHash(const FLocalChunkPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y));
}