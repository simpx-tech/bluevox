#pragma once

#include "CoreMinimal.h"
#include "Piece.generated.h"

USTRUCT(BlueprintType)
struct FPiece
{
	GENERATED_BODY()

	FPiece()
	{
	}

	FPiece(const uint16 InId, const uint16 InSize)
		: Id(InId), Size(InSize)
	{
	}

	UPROPERTY()
	uint16 Id = 0;

	UPROPERTY()
	uint16 Size = 0;

	friend FArchive& operator<<(FArchive& Ar, FPiece& Piece)
	{
		Ar << Piece.Id;
		Ar << Piece.Size;
		return Ar;
	}
};