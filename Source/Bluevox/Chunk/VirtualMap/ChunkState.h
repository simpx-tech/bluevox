#pragma once

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EChunkState : uint8
{
	None = 0 UMETA(Hidden),
	
	Collision = 1 << 0 UMETA(DisplayName = "Collision"),
	Rendered = 1 << 1 UMETA(DisplayName = "Visual"),
	
	LocalLive = 1 << 2 UMETA(DisplayName = "Local Live"),
	LocalFar = 1 << 3 UMETA(DisplayName = "Local Far"),

	Live = Collision | Rendered UMETA(DisplayName = "Live"),
	RemoteLive = Collision UMETA(DisplayName = "Remote Live"),
	VisualOnly = Rendered UMETA(DisplayName = "Visual Only"),
	RemoteVisualOnly = None UMETA(DisplayName = "Remote Visual Only"),
	SoftLoaded = RemoteVisualOnly | Collision UMETA(DisplayName = "Soft Loaded"),
};

ENUM_CLASS_FLAGS(EChunkState);
