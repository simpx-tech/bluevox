#pragma once

#include "CoreMinimal.h"
#include "ColumnPosition.h"
#include "GlobalPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Utils/FloorDiv.h"
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
		const float ChunkWorldSize = GameConstants::Chunk::Size
							   * GameConstants::Scaling::XYWorldSize;
		
		FChunkPosition ChunkPosition;
		ChunkPosition.X = FMath::FloorToInt(Location.X / ChunkWorldSize);
		ChunkPosition.Y = FMath::FloorToInt(Location.Y / ChunkWorldSize);
		return ChunkPosition;
	}

	static FChunkPosition FromGlobalPosition(const FGlobalPosition& GlobalPosition)
	{
		FChunkPosition ChunkPosition;
		ChunkPosition.X = FloorDiv(GlobalPosition.X, GameConstants::Chunk::Size);
		ChunkPosition.Y = FloorDiv(GlobalPosition.Y, GameConstants::Chunk::Size);
		return ChunkPosition;
	}

	static FChunkPosition FromIntVector2(const FIntVector2& IntVector)
	{
		FChunkPosition ChunkPosition;
		ChunkPosition.X = IntVector.X;
		ChunkPosition.Y = IntVector.Y;
		return ChunkPosition;
	}

	static FChunkPosition FromColumnPosition(const FColumnPosition& ColumnPosition)
	{
		FChunkPosition ChunkPosition;
		ChunkPosition.X = FloorDiv(ColumnPosition.X, GameConstants::Chunk::Size);
		ChunkPosition.Y = FloorDiv(ColumnPosition.Y, GameConstants::Chunk::Size);
		return ChunkPosition;
	}

	UPROPERTY(EditAnywhere)
	int32 X = 0;

	UPROPERTY(EditAnywhere)
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

	friend FArchive& operator<<(FArchive& Ar, FChunkPosition& Position)
	{
		Ar << Position.X;
		Ar << Position.Y;
		return Ar;
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