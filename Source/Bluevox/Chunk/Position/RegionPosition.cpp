#include "RegionPosition.h"
#include "ChunkPosition.h"
#include "Bluevox/Utils/FloorDiv.h"

FRegionPosition FRegionPosition::FromChunkPosition(const FChunkPosition& ChunkPosition)
{
	FRegionPosition RegionPosition;

	RegionPosition.X = FloorDiv(ChunkPosition.X, GameConstants::Region::Size);
	RegionPosition.Y = FloorDiv(ChunkPosition.Y, GameConstants::Region::Size);
	
	return RegionPosition;
}
