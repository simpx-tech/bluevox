#pragma once

UENUM(BlueprintType)
enum class EFaceVisibility : uint8
{
	Opaque UMETA(DisplayName = "Opaque"),
	Transparent UMETA(DisplayName = "Transparent"),
	None UMETA(DisplayName = "Visible"),
};
