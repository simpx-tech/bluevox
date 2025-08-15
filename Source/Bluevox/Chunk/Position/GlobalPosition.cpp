#include "GlobalPosition.h"

#include "LocalChunkPosition.h"
#include "LocalPosition.h"

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
