#pragma once

#include "CoreMinimal.h"
#include "GlobalPosition.generated.h"

USTRUCT(BlueprintType)
struct FGlobalPosition
{
	GENERATED_BODY()

	FGlobalPosition()
	{
	}

	static FGlobalPosition FromActorLocation(const FVector& Location)
	{
		FGlobalPosition GlobalPosition;
		GlobalPosition.X = FMath::FloorToInt(Location.X / GameRules::Scaling::XYWorldSize);
		GlobalPosition.Y = FMath::FloorToInt(Location.Y / GameRules::Scaling::XYWorldSize);
		GlobalPosition.Z = FMath::FloorToInt(Location.Z / GameRules::Scaling::ZSize);
		return GlobalPosition;
	}
	
	UPROPERTY()
	int X = 0;

	UPROPERTY()
	int Y = 0;

	UPROPERTY()
	int Z = 0;
};
