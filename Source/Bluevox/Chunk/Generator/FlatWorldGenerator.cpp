// Fill out your copyright notice in the Description page of Project Settings.


#include "FlatWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/GameRules.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/ShapeMaterial/ShapeMaterialRegistry.h"

void UFlatWorldGenerator::GenerateChunk(const FChunkPosition& Position,
                                        UChunkData* OutChunkData) const
{
	OutChunkData->Columns.SetNumUninitialized(GameRules::Chunk::Size * GameRules::Chunk::Size);

	const auto ShapeId = GameManager->ShapeRegistry->GetShapeIdByName(ShapeName);
	const auto MaterialId = GameManager->MaterialRegistry->GetMaterialByName(MaterialName);
	
	for (int32 X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			const int32 Index = X + Y * GameRules::Chunk::Size;
			auto& Column = OutChunkData->Columns[Index];
			
			Column.Pieces.Add(FPiece{
				static_cast<uint16>(
					static_cast<uint16>(MaterialId) << 8 | static_cast<uint16>(ShapeId)
					),
				static_cast<unsigned short>(GroundHeight)
			});
			Column.Pieces.Add(FPiece{
				0,
				static_cast<unsigned short>(GameRules::Chunk::Size - GroundHeight)
			});
		}
	}
}

void UFlatWorldGenerator::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << GroundHeight;
	Ar << ShapeName;
	Ar << MaterialName;
}
