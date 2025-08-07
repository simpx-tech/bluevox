#pragma once

UENUM(BlueprintType)
enum class EFace : uint8
{
	North = 0 UMETA(DisplayName = "North"),
	South = 1 UMETA(DisplayName = "South"),
	East  = 2 UMETA(DisplayName = "East"),
	West = 3 UMETA(DisplayName = "West"),
	Top = 4 UMETA(DisplayName = "Top"),
	Bottom = 5 UMETA(DisplayName = "Bottom"),
	Anywhere = 6 UMETA(DisplayName = "Anywhere")
};

namespace FaceUtils
{
	static constexpr EFace GetOppositeFace(const EFace Face)
	{
		switch (Face)
		{
			case EFace::North: return EFace::South;
			case EFace::South: return EFace::North;
			case EFace::East: return EFace::West;
			case EFace::West: return EFace::East;
			case EFace::Top: return EFace::Bottom;
			case EFace::Bottom: return EFace::Top;
			default: return Face;
		}
	}

	static constexpr FIntVector2 GetHorizontalOffsetByFace(const EFace Face)
	{
		switch (Face)
		{
		case EFace::North: return FIntVector2(1, 0);
		case EFace::South: return FIntVector2(-1, 0);
		case EFace::East: return FIntVector2(0, 1);
		case EFace::West: return FIntVector2(0, -1);
		default: return FIntVector2::ZeroValue;
		}
	}

	static constexpr TStaticArray<EFace, 4> AllHorizontalFaces = {
		EFace::North,
		EFace::South,
		EFace::East,
		EFace::West
	};

	static constexpr TStaticArray<EFace, 6> AllFaces = {
		EFace::North,
		EFace::South,
		EFace::East,
		EFace::West,
		EFace::Top,
		EFace::Bottom
	};
}