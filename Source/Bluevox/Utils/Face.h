#pragma once

UENUM(BlueprintType)
enum class EFace : uint8
{
	North UMETA(DisplayName = "North"),
	South UMETA(DisplayName = "South"),
	East UMETA(DisplayName = "East"),
	West UMETA(DisplayName = "West"),
	Top UMETA(DisplayName = "Top"),
	Bottom UMETA(DisplayName = "Bottom"),
};
