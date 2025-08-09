// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkHelper.h"

#include "Position/ChunkPosition.h"

void UChunkHelper::GetBorderChunks(const FChunkPosition& GlobalPosition, const int32 Distance,
	TSet<FChunkPosition>& OutPositions)
{
	for (int32 X = GlobalPosition.X - Distance; X <= GlobalPosition.X + Distance; ++X)
	{
		for (int32 Y = GlobalPosition.Y - Distance; Y <= GlobalPosition.Y + Distance; ++Y)
		{
			if (FMath::Abs(X - GlobalPosition.X) == Distance || FMath::Abs(Y - GlobalPosition.Y) == Distance)
			{
				OutPositions.Emplace({ X, Y });
			}
		}
	}
}

void UChunkHelper::GetChunksAroundLoadAndLive(const FChunkPosition& GlobalPosition,
                                             const int32 Distance, TSet<FChunkPosition>& OutLoad,
                                             TSet<FChunkPosition>& OutLive)
{
	const auto LiveDistance = Distance - 1;
	
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

			if (FMath::Abs(X - GlobalPosition.X) <= LiveDistance && FMath::Abs(Y - GlobalPosition.Y) <= LiveDistance)
			{
				OutLive.Add(ChunkPosition);
			}
			else
			{
				OutLoad.Add(ChunkPosition);
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

int32 UChunkHelper::GetDistance(const FChunkPosition& A, const FChunkPosition& B)
{
	return FMath::Max(FMath::Abs(A.X - B.X), FMath::Abs(A.Y - B.Y));
}
