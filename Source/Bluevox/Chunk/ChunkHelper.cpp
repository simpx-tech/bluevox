// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkHelper.h"

#include "Position/ChunkPosition.h"

void UChunkHelper::GetChunksAroundLiveAndFar(const FChunkPosition& GlobalPosition,
                                             const int32 FarDistance, const int32 LiveDistance, TSet<FChunkPosition>& OutFar,
                                             TSet<FChunkPosition>& OutLive)
{
	const auto MinX = GlobalPosition.X - FarDistance;
	const auto MaxX = GlobalPosition.X + FarDistance;

	const auto MinY = GlobalPosition.Y - FarDistance;
	const auto MaxY = GlobalPosition.Y + FarDistance;

	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			FChunkPosition ChunkPosition;
			ChunkPosition.X = X;
			ChunkPosition.Y = Y;

			if (FMath::Abs(X - GlobalPosition.X) <= LiveDistance && FMath::Abs(Y - GlobalPosition.Y) <= LiveDistance)
			{
				OutLive.Add(ChunkPosition);
			}
			else
			{
				OutFar.Add(ChunkPosition);
			}
		}
	}
}

void UChunkHelper::GetChunksAround(const FChunkPosition& GlobalPosition, int32 Distance,
	TSet<FChunkPosition>& OutPositions)
{
	const auto MinX = GlobalPosition.X - Distance;
	const auto MaxX = GlobalPosition.X + Distance;

	const auto MinY = GlobalPosition.Y - Distance;
	const auto MaxY = GlobalPosition.Y + Distance;

	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			FChunkPosition ChunkPosition;
			ChunkPosition.X = X;
			ChunkPosition.Y = Y;

			OutPositions.Add(ChunkPosition);
		}
	}
}
