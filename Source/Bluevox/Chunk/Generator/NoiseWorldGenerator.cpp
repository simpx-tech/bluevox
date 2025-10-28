// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "FastNoiseWrapper.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	TMap<EInstanceType, FInstanceCollection> DummyInstances;
	GenerateChunk(Position, OutColumns, DummyInstances);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
                                         TMap<EInstanceType, FInstanceCollection>& OutInstances) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	constexpr auto DirtId = EMaterial::Dirt;
	constexpr auto GrassId = EMaterial::Grass;
	constexpr auto StoneId = EMaterial::Stone;

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

			Column.Pieces.Emplace(EMaterial::Void, VoidHeight);
		}
	}

	// Generate instances after terrain generation
	GenerateInstances(Position, OutColumns, OutInstances);
}

void UNoiseWorldGenerator::GenerateInstances(const FChunkPosition& Position, const TArray<FChunkColumn>& Columns,
                                             TMap<EInstanceType, FInstanceCollection>& OutInstances) const
{
	OutInstances.Empty();

	// Create tree instance collection
	FInstanceCollection& TreeCollection = OutInstances.Add(EInstanceType::Tree, FInstanceCollection(EInstanceType::Tree));

	// Use position-based seed for deterministic random generation
	const int32 Seed = Position.X * 73856093 + Position.Y * 19349663;
	FRandomStream RandomStream(Seed);

	// Track placed tree positions for collision detection
	TArray<FVector2D> PlacedTreePositions;
	constexpr float MinTreeDistance = 3.0f; // Minimum distance between trees in blocks

	for (int32 X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			// Random chance to spawn a tree
			if (RandomStream.FRand() > TreeSpawnChance)
			{
				continue;
			}

			const int32 Index = UChunkData::GetIndex(X, Y);
			const FChunkColumn& Column = Columns[Index];

			// Find the surface (first non-void from top)
			int32 SurfaceZ = -1;
			int32 CurrentZ = 0;
			EMaterial SurfaceMaterial = EMaterial::Void;

			for (const FPiece& Piece : Column.Pieces)
			{
				if (Piece.MaterialId != EMaterial::Void)
				{
					// This is a solid piece, the top of it is the surface
					SurfaceZ = CurrentZ + Piece.Size;
					SurfaceMaterial = Piece.MaterialId;
				}
				CurrentZ += Piece.Size;
			}

			// Only place trees on grass surfaces
			if (SurfaceZ < 0 || SurfaceMaterial != EMaterial::Grass)
			{
				continue;
			}

			// Check if there's enough space above for the tree
			// Find how much void space is above the surface
			int32 VoidAboveSurface = 0;
			CurrentZ = 0;
			for (const FPiece& Piece : Column.Pieces)
			{
				if (CurrentZ >= SurfaceZ && Piece.MaterialId == EMaterial::Void)
				{
					VoidAboveSurface = Piece.Size;
					break;
				}
				CurrentZ += Piece.Size;
			}

			// Ensure there's enough space for the tree
			if (VoidAboveSurface < TreeHeightInLayers)
			{
				continue;
			}

			// Check collision with existing trees
			const FVector2D CurrentPos(X, Y);
			bool bTooCloseToOtherTree = false;
			for (const FVector2D& PlacedPos : PlacedTreePositions)
			{
				const float DistanceSquared = FVector2D::DistSquared(CurrentPos, PlacedPos);
				if (DistanceSquared < MinTreeDistance * MinTreeDistance)
				{
					bTooCloseToOtherTree = true;
					break;
				}
			}

			if (bTooCloseToOtherTree)
			{
				continue;
			}

			// Create tree instance at the surface
			const FVector Location(X, Y, SurfaceZ);

			// Random rotation around Z axis
			const float RandomYaw = RandomStream.FRand() * 360.0f;
			const FRotator Rotation(0.0f, RandomYaw, 0.0f);

			// Random scale variation
			const float ScaleVariation = RandomStream.FRandRange(0.8f, 1.2f);
			const FVector Scale(ScaleVariation, ScaleVariation, ScaleVariation);

			TreeCollection.AddInstance(FInstanceData(Location, Rotation, Scale));
			PlacedTreePositions.Add(CurrentPos);
		}
	}
}
