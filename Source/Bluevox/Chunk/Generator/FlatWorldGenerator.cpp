// Fill out your copyright notice in the Description page of Project Settings.


#include "FlatWorldGenerator.h"

#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameConstants.h"

void UFlatWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                        TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	// const auto ShapeId = GameManager->ShapeRegistry->GetShapeIdByName(ShapeName);

	for (int32 X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int32 Index = X + Y * GameConstants::Chunk::Size;
			auto& Column = OutColumns[Index];

			Column.Pieces.Add(FPiece{
				Shape,
				static_cast<unsigned short>(GroundHeight)
			});
			Column.Pieces.Add(FPiece{
				EMaterial::Void,
				static_cast<unsigned short>(GameConstants::Chunk::Height - GroundHeight)
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
