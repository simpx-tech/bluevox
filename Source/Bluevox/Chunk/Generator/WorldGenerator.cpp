// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Entity/EntityTypes.h"

UWorldGenerator* UWorldGenerator::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	return this;
}

void UWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                    TArray<FChunkColumn>& OutColumns,
                                    TArray<FEntityRecord>& OutEntities)
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	for (int32 X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int32 Index = UChunkData::GetIndex(X, Y);
			auto& Column = OutColumns[Index];

			Column.Pieces.Add(FPiece{
				EMaterial::Void,
				static_cast<unsigned short>(GameConstants::Chunk::Height)
			});
		}
	}

	// Base implementation doesn't generate entities
	OutEntities.Empty();
}
