#include "RegionPosition.h"
#include "ChunkPosition.h"

FRegionPosition FRegionPosition::FromChunkPosition(const FChunkPosition& ChunkPosition)
{
	FRegionPosition RegionPosition;
	RegionPosition.X = ChunkPosition.X / GameRules::Chunk::Size;
	RegionPosition.Y = ChunkPosition.Y / GameRules::Chunk::Size;
	
	return RegionPosition;
}
