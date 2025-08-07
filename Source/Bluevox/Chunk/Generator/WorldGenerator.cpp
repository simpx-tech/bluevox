// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"

UWorldGenerator* UWorldGenerator::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	return this;
}

void UWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                    TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameRules::Chunk::Size * GameRules::Chunk::Size);

	for (int32 X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			const int32 Index = UChunkData::GetIndex(X, Y);
			auto& Column = OutColumns[Index];
			
			Column.Pieces.Add(FPiece{
				0,
				static_cast<unsigned short>(GameRules::Chunk::Height)
			});
		}
	}
}
