#pragma once

#include "CoreMinimal.h"
#include "VoxelMaterial.generated.h"

UENUM(BlueprintType)
enum class EMaterial : uint8
{
	Void UMETA(DisplayName = "Void"),
	Dirt UMETA(DisplayName = "Dirt"),
	Grass UMETA(DisplayName = "Grass"),
	Stone UMETA(DisplayName = "Stone"),
	Sand UMETA(DisplayName = "Sand"),
	Snow UMETA(DisplayName = "Snow"),
	Water UMETA(DisplayName = "Water"),
};
