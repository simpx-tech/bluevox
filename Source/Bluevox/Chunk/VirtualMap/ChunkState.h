#pragma once

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EChunkState : uint8
{
	None = 0 UMETA(Hidden),
	Visible = 1 << 0 UMETA(DisplayName = "Visible"),
	Collision = 1 << 1 UMETA(DisplayName = "Collision"),

	Live = Visible | Collision UMETA(DisplayName = "Live"),
	RemoteLive = Collision UMETA(DisplayName = "Remote Live"),
	LoadOnly = None UMETA(DisplayName = "Load Only"),
};

ENUM_CLASS_FLAGS(EChunkState);
