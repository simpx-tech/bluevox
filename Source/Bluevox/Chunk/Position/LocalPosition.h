#pragma once

#include "CoreMinimal.h"
#include "LocalPosition.generated.h"

USTRUCT(BlueprintType)
struct FLocalPosition
{
	GENERATED_BODY()

	FLocalPosition()
	{
	}

	FLocalPosition(const uint8 InX, const uint8 InY, const uint16 InZ)
		: X(InX), Y(InY), Z(InZ)
	{
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