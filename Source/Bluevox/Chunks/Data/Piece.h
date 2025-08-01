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
