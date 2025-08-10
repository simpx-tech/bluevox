// Fill out your copyright notice in the Description page of Project Settings.


#include "FlatWorldGenerator.h"

#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameRules.h"
#include "Bluevox/Shape/ShapeRegistry.h"

void UFlatWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                        TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameRules::Chunk::Size * GameRules::Chunk::Size);

	const auto ShapeId = GameManager->ShapeRegistry->GetShapeIdByName(ShapeName);

	for (int32 X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			const int32 Index = X + Y * GameRules::Chunk::Size;
			auto& Column = OutColumns[Index];

			Column.Pieces.Add(FPiece{
				ShapeId,
				static_cast<unsigned short>(GroundHeight)
			});
			Column.Pieces.Add(FPiece{
				0,
				static_cast<unsigned short>(GameRules::Chunk::Height - GroundHeight)
			});
		}
	}
}

void UFlatWorldGenerator::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << GroundHeight;
	Ar << ShapeName;
}
