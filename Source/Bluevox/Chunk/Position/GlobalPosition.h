#pragma once

#include "CoreMinimal.h"
#include "ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "GlobalPosition.generated.h"

struct FLocalPosition;
struct FChunkPosition;

USTRUCT(BlueprintType)
struct FGlobalPosition
{
	GENERATED_BODY()

	FGlobalPosition()
	{
	}

	FGlobalPosition(const int InX, const int InY, const int InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}
	
	static FGlobalPosition FromActorLocation(const FVector& Location)
	{
		FGlobalPosition GlobalPosition;
		GlobalPosition.X = FMath::FloorToInt(Location.X / GameConstants::Scaling::XYWorldSize);
		GlobalPosition.Y = FMath::FloorToInt(Location.Y / GameConstants::Scaling::XYWorldSize);
		GlobalPosition.Z = FMath::FloorToInt(Location.Z / GameConstants::Scaling::ZWorldSize);
		return GlobalPosition;
	}

	static FGlobalPosition FromLocalPosition(const FChunkPosition& ChunkPosition, const FLocalPosition& LocalPosition);

	bool IsBorderBlock() const;

	void GetBorderChunks(TArray<FChunkPosition>& OutPositions) const;
	
	UPROPERTY()
	int X = 0;

	UPROPERTY()
	int Y = 0;

	UPROPERTY()
	int Z = 0;

	friend FArchive& operator<<(FArchive& Ar, FGlobalPosition& Pos)
	{
		Ar << Pos.X;
		Ar << Pos.Y;
		Ar << Pos.Z;
		return Ar;
	}

	FGlobalPosition operator+(const FIntVector3& Other) const
	{
		return FGlobalPosition(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	FGlobalPosition operator-(const FIntVector3& Other) const
	{
		return FGlobalPosition(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	FGlobalPosition operator+(const FGlobalPosition& Other) const
	{
		return FGlobalPosition(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	FGlobalPosition& operator+=(const FGlobalPosition& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		return *this;
	}

	FGlobalPosition operator-(const FGlobalPosition& Other) const
	{
		return FGlobalPosition(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("GlobalPosition(%d, %d, %d)"), X, Y, Z);
	}

	FVector AsActorLocationCopy() const
	{
		return FVector(
			X * GameConstants::Scaling::XYWorldSize,
			Y * GameConstants::Scaling::XYWorldSize,
			Z * GameConstants::Scaling::ZWorldSize);
	}
};

FORCEINLINE uint32 GetTypeHash(const FGlobalPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y), GetTypeHash(Pos.Z));
}