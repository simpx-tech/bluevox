#include "LocalPosition.h"

#include "GlobalPosition.h"

FLocalPosition FLocalPosition::FromGlobalPosition(const FGlobalPosition& GlobalPosition)
{
	FLocalPosition LocalPosition;
	LocalPosition.X = static_cast<uint8>(PositiveMod(GlobalPosition.X, GameConstants::Chunk::Size));
	LocalPosition.Y = static_cast<uint8>(PositiveMod(GlobalPosition.Y, GameConstants::Chunk::Size));
	LocalPosition.Z = static_cast<uint16>(PositiveMod(GlobalPosition.Z, GameConstants::Chunk::Height));

	return LocalPosition;
}
