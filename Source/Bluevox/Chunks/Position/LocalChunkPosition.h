#pragma once

#include "CoreMinimal.h"
#include "ChunkPosition.h"
#include "LocalChunkPosition.generated.h"

USTRUCT(BlueprintType)
struct FLocalChunkPosition
{
	GENERATED_BODY()

	FLocalChunkPosition()
	{
	}

	static FLocalChunkPosition FromChunkPosition(const FChunkPosition& ChunkPosition)
	{
		FLocalChunkPosition LocalChunkPosition;
		LocalChunkPosition.X = ChunkPosition.X % GameRules::Chunk::Size;
		LocalChunkPosition.Y = ChunkPosition.Y % GameRules::Chunk::Size;
		return LocalChunkPosition;
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