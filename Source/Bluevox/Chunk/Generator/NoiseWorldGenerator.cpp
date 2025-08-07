// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameRules.h"
#include "FastNoiseWrapper.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/ShapeMaterial/ShapeMaterialRegistry.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	OutColumns.SetNum(GameRules::Chunk::Size * GameRules::Chunk::Size);

	const auto ShapeId = GameManager->ShapeRegistry->GetShapeIdByName(ShapeName);
	const auto MaterialId = GameManager->MaterialRegistry->GetMaterialByName(MaterialName);

	for (int X = 0; X < GameRules::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameRules::Chunk::Size; ++Y)
		{
			const auto Index = UChunkData::GetIndex(X, Y);
			auto& Column = OutColumns[Index];

			const auto WorldX = X + Position.X * GameRules::Chunk::Size;
			const auto WorldY = Y + Position.Y * GameRules::Chunk::Size;

			const float NoiseValue = Noise->GetNoise2D(WorldX * 0.5f, WorldY * 0.5f);

			const unsigned short Height = FMath::RoundToInt(
				(NoiseValue + 1) * (GameRules::Chunk::Height / 2));
			Column.Pieces.Emplace(static_cast<uint16>(
					static_cast<uint16>(MaterialId) << 8 | static_cast<uint16>(ShapeId)
					), Height);
			Column.Pieces.Emplace(0, GameRules::Chunk::Height - Height);
		}
	}
}
