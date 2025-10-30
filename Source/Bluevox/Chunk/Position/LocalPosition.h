#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Utils/PositiveMod.h"
#include "LocalPosition.generated.h"

struct FGlobalPosition;

USTRUCT(BlueprintType)
struct FLocalPosition
{
	GENERATED_BODY()

	FLocalPosition()
	{
	}

	FLocalPosition(const uint8 InX, const uint8 InY, const uint16 InZ) {
		X = static_cast<uint8>(PositiveMod(InX, GameConstants::Chunk::Size));
		Y = static_cast<uint8>(PositiveMod(InY, GameConstants::Chunk::Size));
		Z = static_cast<uint16>(InZ);
	}

	static FLocalPosition FromActorLocation(const FVector& Location)
	{
		const int32 GlobalX = FMath::FloorToInt(Location.X / GameConstants::Scaling::XYWorldSize);
		const int32 GlobalY = FMath::FloorToInt(Location.Y / GameConstants::Scaling::XYWorldSize);
		const int32 GlobalZ = FMath::FloorToInt(Location.Z / GameConstants::Scaling::ZWorldSize);

		FLocalPosition LocalPosition;
		LocalPosition.X = static_cast<uint8>(PositiveMod(GlobalX, GameConstants::Chunk::Size));
		LocalPosition.Y = static_cast<uint8>(PositiveMod(GlobalY, GameConstants::Chunk::Size));
		LocalPosition.Z = static_cast<uint16>(GlobalZ);

		return LocalPosition;
	}

	static FLocalPosition FromGlobalPosition(const FGlobalPosition& GlobalPosition);

	FString ToString() const
	{
		return FString::Printf(TEXT("LocalPosition(X: %d, Y: %d, Z: %d)"), X, Y, Z);
	}

	FLocalPosition operator+(const FIntVector3& Other) const
	{
		return FLocalPosition(
			static_cast<uint8>(PositiveMod(X + Other.X, GameConstants::Chunk::Size)),
			static_cast<uint8>(PositiveMod(Y + Other.Y, GameConstants::Chunk::Size)),
			static_cast<uint16>(Z + Other.Z));
	}

	bool operator==(const FLocalPosition& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}

	FLocalPosition operator+(const FLocalPosition& Other) const
	{
		return FLocalPosition(
			static_cast<uint8>(PositiveMod(X + Other.X, GameConstants::Chunk::Size)),
			static_cast<uint8>(PositiveMod(Y + Other.Y, GameConstants::Chunk::Size)),
			static_cast<uint16>(Z + Other.Z));
	}

	FLocalPosition operator-(const FLocalPosition& Other) const
	{
		return FLocalPosition(
			static_cast<uint8>(PositiveMod(X - Other.X, GameConstants::Chunk::Size)),
			static_cast<uint8>(PositiveMod(Y - Other.Y, GameConstants::Chunk::Size)),
			static_cast<uint16>(Z - Other.Z));
	}

	friend FArchive& operator<<(FArchive& Ar, FLocalPosition& Pos)
	{
		Ar << Pos.X;
		Ar << Pos.Y;
		Ar << Pos.Z;
		return Ar;
	}

	UPROPERTY()
	uint8 X = 0;

	UPROPERTY()
	uint8 Y = 0;

	UPROPERTY()
	uint16 Z = 0;
};

FORCEINLINE uint32 GetTypeHash(const FLocalPosition& Pos)
{
	return HashCombine(GetTypeHash(Pos.X), GetTypeHash(Pos.Y), GetTypeHash(Pos.Z));
}