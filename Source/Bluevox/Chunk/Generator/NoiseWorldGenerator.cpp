// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"
#include "FastNoiseWrapper.h"
#include "Engine/AssetManager.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("Noise"));
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 123, 0.03f);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	TMap<FPrimaryAssetId, FInstanceCollection> DummyInstances;
	GenerateChunk(Position, OutColumns, DummyInstances);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
                                         TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const
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
                                             TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const
{
	OutInstances.Empty();

	// Process each configured instance type
	for (const auto& AssetPtr : InstanceTypesToGenerate)
	{
		UInstanceTypeDataAsset* Asset = AssetPtr.LoadSynchronous();
		if (!Asset)
		{
			continue;
		}

		FPrimaryAssetId AssetId = Asset->GetPrimaryAssetId();
		if (!AssetId.IsValid())
		{
			// If asset doesn't have a valid primary asset ID, create one from its path
			AssetId = FPrimaryAssetId(UInstanceTypeDataAsset::StaticClass()->GetFName(),
				FName(*Asset->GetPathName()));
		}

		// Create collection for this instance type
		FInstanceCollection& Collection = OutInstances.FindOrAdd(AssetId);
		Collection.InstanceTypeId = AssetId;

		// Generate instances for this type
		GenerateInstancesOfType(Asset, Position, Columns, Collection.Instances);

		// Rebuild cache for efficient rendering
		Collection.RebuildTransformCache();
	}
}

void UNoiseWorldGenerator::GenerateInstancesOfType(UInstanceTypeDataAsset* Asset, const FChunkPosition& Position,
                                                   const TArray<FChunkColumn>& Columns, TArray<FInstanceData>& OutInstances) const
{
	if (!Asset)
	{
		return;
	}

	// Use position-based seed for deterministic random generation
	const int32 Seed = Position.X * 73856093 + Position.Y * 19349663 + GetTypeHash(Asset->GetPrimaryAssetId());
	FRandomStream RandomStream(Seed);

	// Track placed instance positions for collision detection
	TArray<FVector2D> PlacedPositions;

	for (int32 X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			// Random chance to spawn based on asset configuration
			if (RandomStream.FRand() > Asset->SpawnChance)
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

			// Check if surface material is valid for this instance type
			if (SurfaceZ < 0 || !Asset->ValidSurfaces.Contains(SurfaceMaterial))
			{
				continue;
			}

			// Check if there's enough space above
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

			// Ensure there's enough space based on asset configuration
			if (VoidAboveSurface < Asset->RequiredVoidSpace)
			{
				continue;
			}

			// Check collision with existing instances
			const FVector2D CurrentPos(X, Y);
			bool bTooClose = false;
			for (const FVector2D& PlacedPos : PlacedPositions)
			{
				const float DistanceSquared = FVector2D::DistSquared(CurrentPos, PlacedPos);
				if (DistanceSquared < Asset->MinSpacing * Asset->MinSpacing)
				{
					bTooClose = true;
					break;
				}
			}

			if (bTooClose)
			{
				continue;
			}

			// Create instance at the surface
			const FVector Location(X, Y, SurfaceZ);

			// Random rotation around Z axis
			const float RandomYaw = RandomStream.FRand() * 360.0f;
			const FRotator Rotation(0.0f, RandomYaw, 0.0f);

			// Random scale variation based on asset configuration
			const float ScaleVariation = RandomStream.FRandRange(Asset->MinScale, Asset->MaxScale);
			const FVector Scale(ScaleVariation, ScaleVariation, ScaleVariation);

			// Create instance with transform and asset reference
			FTransform Transform(Rotation, Location, Scale);
			// Create TSoftObjectPtr from asset path rather than loaded asset
			TSoftObjectPtr<UInstanceTypeDataAsset> AssetRef(FSoftObjectPath(Asset->GetPathName()));
			OutInstances.Add(FInstanceData(Transform, AssetRef));

			PlacedPositions.Add(CurrentPos);
		}
	}
}
