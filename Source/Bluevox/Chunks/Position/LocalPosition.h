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

	UPROPERTY()
	uint8 X = 0;

	UPROPERTY()
	uint8 Y = 0;

	UPROPERTY()
	uint8 Z = 0;
};
