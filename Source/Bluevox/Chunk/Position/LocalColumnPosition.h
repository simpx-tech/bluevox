#pragma once

#include "CoreMinimal.h"
#include "ColumnPosition.h"
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
		LocalPosition.X = ColumnPosition.X;
		LocalPosition.Y = ColumnPosition.Y;
		return LocalPosition;
	}
	
	UPROPERTY()
	uint16 X = 0;

	UPROPERTY()
	uint16 Y = 0;
};
