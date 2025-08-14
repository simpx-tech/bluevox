#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/GameRules.h"
#include "GlobalPosition.generated.h"

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
		GlobalPosition.X = FMath::FloorToInt(Location.X / GameRules::Scaling::XYWorldSize);
		GlobalPosition.Y = FMath::FloorToInt(Location.Y / GameRules::Scaling::XYWorldSize);
		GlobalPosition.Z = FMath::FloorToInt(Location.Z / GameRules::Scaling::ZSize);
		return GlobalPosition;
	}

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

	FString ToString() const
	{
		return FString::Printf(TEXT("GlobalPosition(%d, %d, %d)"), X, Y, Z);
	}

	FVector AsActorLocationCopy() const
	{
		return FVector(
			X * GameRules::Scaling::XYWorldSize,
			Y * GameRules::Scaling::XYWorldSize,
			Z * GameRules::Scaling::ZSize);
	}
};

FORCEINLINE uint32 GetTypeHash(const FGlobalPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y), GetTypeHash(Pos.Z));
}