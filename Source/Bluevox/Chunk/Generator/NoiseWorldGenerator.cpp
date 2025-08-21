// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "FastNoiseWrapper.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/ShapeRegistry.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	const auto DirtId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Dirt);
	const auto GrassId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Grass);
	const auto StoneId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Layer_Stone);
	const auto WaterId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Shapes::GShape_Water);
	
	const auto MaxHeight = FMath::FloorToInt(GameConstants::Chunk::Height * 0.75f);
	
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const auto Index = UChunkData::GetIndex(X, Y);
			auto& Column = OutColumns[Index];

			const auto WorldX = X + Position.X * GameConstants::Chunk::Size;
			const auto WorldY = Y + Position.Y * GameConstants::Chunk::Size;

			const float NoiseValue = Noise->GetNoise2D(WorldX * 0.5f, WorldY * 0.5f);

			const unsigned short Height = FMath::RoundToInt(
				(NoiseValue + 1) * (MaxHeight / 2));

			const auto GrassHeight = FMath::RandRange(1, 2);
			const auto DirtHeight = FMath::RandRange(1, 6);
			const auto StoneHeight = Height - GrassHeight - DirtHeight;
			const auto VoidHeight = GameConstants::Chunk::Height - Height;
			
			Column.Pieces.Emplace(StoneId, StoneHeight);
			Column.Pieces.Emplace(DirtId, DirtHeight);
			Column.Pieces.Emplace(GrassId, GrassHeight);

			const auto WaterAmount = FMath::Max(WaterLevel - (GrassHeight + DirtHeight + StoneHeight), 0);
			if (WaterAmount > 0)
			{
				Column.Pieces.Emplace(WaterId, WaterAmount);
			}
			
			Column.Pieces.Emplace(GameConstants::Shapes::GShapeId_Void, VoidHeight - WaterAmount);
		}
	}
}
