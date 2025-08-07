#pragma once

#include "CoreMinimal.h"
#include "Piece.h"
#include "ChunkColumn.generated.h"

USTRUCT(BlueprintType)
struct FChunkColumn
{
	GENERATED_BODY()

	FChunkColumn()
	{
	}

	UPROPERTY()
	TArray<FPiece> Pieces;

	friend FArchive& operator<<(FArchive& Ar, FChunkColumn& Column)
	{
		Ar << Column.Pieces;
		return Ar;
	}
};