// Fill out your copyright notice in the Description page of Project Settings.


#include "FlatWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Game/GameRules.h"

void UFlatWorldGenerator::GenerateChunk(const FLocalChunkPosition& Position,
                                        UChunkData* OutChunkData) const
{
	OutChunkData->Columns.SetNumUninitialized(GameRules::Chunk::Size * GameRules::Chunk::Size);

	const auto BlockPaletteId = OutChunkData->RegisterBlock(MaterialId);
	
	for (int32 X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			const int32 Index = X + Y * GameRules::Chunk::Size;
			auto& Column = OutChunkData->Columns[Index];

			
			
			Column.Pieces.Add(FPiece{});
			Column.Pieces.Add(FPiece{});
		}
	}
}

void UFlatWorldGenerator::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << GroundHeight;
}
