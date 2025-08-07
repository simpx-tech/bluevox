#pragma once

#include "CoreMinimal.h"
#include "ColumnPosition.h"
#include "Bluevox/Utils/PositiveMod.h"
#include "Bluevox/Game/GameRules.h"
#include "LocalColumnPosition.generated.h"

USTRUCT(BlueprintType)
struct FLocalColumnPosition
{
	GENERATED_BODY()

	FLocalColumnPosition()
	{
	}

	static FLocalColumnPosition FromColumnPosition(const FColumnPosition& ColumnPosition)
	{
		FLocalColumnPosition LocalPosition;
		LocalPosition.X = PositiveMod(ColumnPosition.X, GameRules::Chunk::Size);
		LocalPosition.Y = PositiveMod(ColumnPosition.Y, GameRules::Chunk::Size);
		return LocalPosition;
	}
	
	UPROPERTY()
	uint16 X = 0;

	UPROPERTY()
	uint16 Y = 0;
};
