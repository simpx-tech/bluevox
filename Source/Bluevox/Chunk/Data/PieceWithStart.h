#pragma once

#include "CoreMinimal.h"
#include "Piece.h"
#include "PieceWithStart.generated.h"

USTRUCT(BlueprintType)
struct FPieceWithStart : public FPiece
{
	GENERATED_BODY()

	FPieceWithStart()
	{
	}

	FPieceWithStart(const uint16 InId, const uint16 InSize, const uint16 InStart)
		: FPiece(InId, InSize), Start(InStart)
	{
	}

	FPieceWithStart(const FPiece& Piece, const uint16 InStart)
		: FPiece(Piece), Start(InStart)
	{
	}

	UPROPERTY()
	uint16 Start = 0;
};
