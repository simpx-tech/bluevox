#pragma once

#include "CoreMinimal.h"
#include "GlobalPosition.h"
#include "Bluevox/Game/GameRules.h"
#include "ChunkPosition.generated.h"

USTRUCT(BlueprintType)
struct FChunkPosition
{
	GENERATED_BODY()

	FChunkPosition()
	{
	}

	FChunkPosition(const int32 InX, const int32 InY)
		: X(InX), Y(InY)
	{
	}

	static FChunkPosition FromActorLocation(const FVector& Location)
	{
		FChunkPosition ChunkPosition;
		ChunkPosition.X = FMath::FloorToInt(Location.X / GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize);
		ChunkPosition.Y = FMath::FloorToInt(Location.Y / GameRules::Chunk::Size * GameRules::Scaling::XYWorldSize);
		return ChunkPosition;
	}

	static FChunkPosition FromGlobalPosition(const FGlobalPosition& GlobalPosition)
	{
		FChunkPosition ChunkPosition;
		ChunkPosition.X = GlobalPosition.X / GameRules::Chunk::Size;
		ChunkPosition.Y = GlobalPosition.Y / GameRules::Chunk::Size;
		return ChunkPosition;
	}

	UPROPERTY()
	int32 X = 0;

	UPROPERTY()
	int32 Y = 0;

	bool operator==(const FChunkPosition& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	FChunkPosition operator+(const FChunkPosition& Other) const
	{
		return FChunkPosition(X + Other.X, Y + Other.Y);
	}

	FChunkPosition& operator+=(const FChunkPosition& Other)
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("ChunkPosition(X: %d, Y: %d)"), X, Y);
	}
};

FORCEINLINE uint32 GetTypeHash(const FChunkPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y));
}