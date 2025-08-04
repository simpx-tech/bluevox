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

	FPiece(const uint16 InPaletteId, const uint16 InSize)
		: PaletteId(InPaletteId), Size(InSize)
	{
	}

	UPROPERTY()
	uint16 PaletteId = 0;

	UPROPERTY()
	uint16 Size = 0;

	friend FArchive& operator<<(FArchive& Ar, FPiece& Piece)
	{
		Ar << Piece.PaletteId;
		Ar << Piece.Size;
		return Ar;
	}
};
