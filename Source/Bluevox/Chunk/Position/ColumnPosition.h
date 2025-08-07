#pragma once

#include "CoreMinimal.h"
#include "ColumnPosition.generated.h"

USTRUCT(BlueprintType)
struct FColumnPosition
{
	GENERATED_BODY()

	FColumnPosition()
	{
	}

	FColumnPosition(const int32 InX, const int32 InY)
		: X(InX), Y(InY)
	{
	}

	UPROPERTY()
	int32 X = 0;

	UPROPERTY()
	int32 Y = 0;

	FString ToString() const
	{
		return FString::Printf(TEXT("ColumnPosition(X: %d, Y: %d)"), X, Y);
	}

	FColumnPosition operator+(const FIntVector2& IntVector2) const
	{
		return FColumnPosition(X + IntVector2.X, Y + IntVector2.Y);
	}
};
