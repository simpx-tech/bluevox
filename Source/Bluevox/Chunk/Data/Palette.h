#pragma once

#include "CoreMinimal.h"
#include "Palette.generated.h"

USTRUCT(BlueprintType)
struct FPalette
{
	GENERATED_BODY()

	FPalette()
	{
	}

	UPROPERTY()
	TArray<uint16> Palette;
	
	UPROPERTY()
	TArray<TPair<uint8, uint8>> BasicShapes;

	// DEV entities (EntityPtr)
	// DEV remote entities (GlobalPosition, EntityId)
};
