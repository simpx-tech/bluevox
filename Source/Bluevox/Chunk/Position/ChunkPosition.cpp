#include "ChunkPosition.h"

#include "GlobalPosition.h"

FChunkPosition FChunkPosition::FromGlobalPosition(const FGlobalPosition& GlobalPosition)
{
	FChunkPosition ChunkPosition;
	ChunkPosition.X = FloorDiv(GlobalPosition.X, GameConstants::Chunk::Size);
	ChunkPosition.Y = FloorDiv(GlobalPosition.Y, GameConstants::Chunk::Size);
	return ChunkPosition;
}
