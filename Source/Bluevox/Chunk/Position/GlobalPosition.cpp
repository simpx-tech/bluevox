#include "GlobalPosition.h"

#include "LocalChunkPosition.h"
#include "LocalPosition.h"

FGlobalPosition FGlobalPosition::FromLocalPosition(const FChunkPosition& ChunkPosition, const FLocalPosition& LocalPosition)
{
	FGlobalPosition GlobalPosition;
	GlobalPosition.X = LocalPosition.X + GameConstants::Chunk::Size * ChunkPosition.X;
	GlobalPosition.Y = LocalPosition.Y + GameConstants::Chunk::Size * ChunkPosition.Y;
	GlobalPosition.Z = LocalPosition.Z;
	return GlobalPosition;
}

bool FGlobalPosition::IsBorderBlock() const
{
	const auto [LocalX, LocalY, LocalZ] = FLocalPosition::FromGlobalPosition(*this);
	
	return LocalX == 0 || LocalY == 0
		|| LocalX == GameConstants::Chunk::Size || LocalY == GameConstants::Chunk::Size;
}

void FGlobalPosition::GetBorderChunks(TArray<FChunkPosition>& OutPositions) const
{
	const auto [LocalX, LocalY, LocalZ] = FLocalPosition::FromGlobalPosition(*this);
	const auto LocalPosition = FChunkPosition::FromGlobalPosition(*this);
	if (LocalX == 0)
	{
		OutPositions.Add(FChunkPosition(LocalPosition.X - 1, LocalPosition.Y));
	} else if (LocalX == GameConstants::Chunk::Size - 1)
	{
		OutPositions.Add(FChunkPosition(LocalPosition.X + 1, LocalPosition.Y));
	}
	
	if (LocalY == 0)
	{
		OutPositions.Add(FChunkPosition(LocalPosition.X, LocalPosition.Y - 1));
	} else if (LocalY == GameConstants::Chunk::Size - 1)
	{
		OutPositions.Add(FChunkPosition(LocalPosition.X, LocalPosition.Y + 1));
	}
}
