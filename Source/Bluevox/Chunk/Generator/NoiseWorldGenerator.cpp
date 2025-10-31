// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"
#include "Bluevox/Entity/EntityTypes.h"
#include "FastNoiseWrapper.h"

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	// Continental noise - large scale terrain features
	ContinentalNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("ContinentalNoise"));
	ContinentalNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		1337,                                  // seed
		TerrainScale,                          // frequency
		EFastNoise_Interp::Quintic,            // interpolation
		EFastNoise_FractalType::FBM,           // fractal type
		4,                                      // octaves
		2.2f,                                   // lacunarity
		0.5f                                    // gain
	);

	// Erosion noise - simulates natural erosion patterns
	ErosionNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("ErosionNoise"));
	ErosionNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		2468,                                   // seed
		TerrainScale * 2.0f,                   // frequency
		EFastNoise_Interp::Quintic,            // interpolation
		EFastNoise_FractalType::Billow,        // fractal type - billow for erosion patterns
		3,                                      // octaves
		2.0f,                                   // lacunarity
		0.45f                                   // gain
	);

	// Peaks noise - sharp mountain features
	PeaksNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("PeaksNoise"));
	PeaksNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		9876,                                   // seed
		TerrainScale * 1.5f,                   // frequency
		EFastNoise_Interp::Quintic,            // interpolation
		EFastNoise_FractalType::RigidMulti,    // fractal type - rigid for sharp peaks
		5,                                      // octaves
		2.5f,                                   // lacunarity
		0.6f                                    // gain
	);

	// Detail noise - small scale terrain details
	DetailNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("DetailNoise"));
	DetailNoise->SetupFastNoise(
		EFastNoise_NoiseType::Simplex,
		5432,                                   // seed
		TerrainScale * 10.0f,                  // frequency
		EFastNoise_Interp::Quintic             // interpolation
	);

	// Temperature noise - controls temperature distribution
	TemperatureNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("TemperatureNoise"));
	TemperatureNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		8888,                                   // seed
		TerrainScale * 0.3f,                   // large scale temperature zones
		EFastNoise_Interp::Quintic,
		EFastNoise_FractalType::FBM,
		2,                                      // octaves
		2.0f,                                   // lacunarity
		0.5f                                    // gain
	);

	// Precipitation noise - controls moisture distribution
	PrecipitationNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("PrecipitationNoise"));
	PrecipitationNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		9999,                                   // seed
		TerrainScale * 0.4f,                   // precipitation patterns
		EFastNoise_Interp::Quintic,
		EFastNoise_FractalType::Billow,        // billow for cloud-like patterns
		2,                                      // octaves
		2.0f,                                   // lacunarity
		0.6f                                    // gain
	);

	// Worley noise - creates cellular patterns for mountain variation
	WorleyNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("WorleyNoise"));
	WorleyNoise->SetupFastNoise(
		EFastNoise_NoiseType::Cellular,
		3333,                                   // seed
		TerrainScale * 2.0f,                   // frequency
		EFastNoise_Interp::Quintic,
		EFastNoise_FractalType::FBM,
		1,                                      // octaves
		2.0f,                                   // lacunarity
		0.5f,                                   // gain
		0.3f,                                   // cellular jitter
		EFastNoise_CellularDistanceFunction::Euclidean,
		EFastNoise_CellularReturnType::Distance
	);

	// Ridge noise - creates ridge patterns for mountains
	RidgeNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("RidgeNoise"));
	RidgeNoise->SetupFastNoise(
		EFastNoise_NoiseType::SimplexFractal,
		4444,                                   // seed
		TerrainScale * 3.0f,                   // frequency
		EFastNoise_Interp::Quintic,
		EFastNoise_FractalType::RigidMulti,    // rigid for sharp ridges
		3,                                      // octaves
		2.0f,                                   // lacunarity
		0.5f                                    // gain
	);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns, TArray<FEntityRecord>& OutEntities) const
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	// Store heights for erosion simulation
	TArray<float> Heights;
	Heights.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);

	// First pass: Generate base terrain heights
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const float WorldX = X + Position.X * GameConstants::Chunk::Size;
			const float WorldY = Y + Position.Y * GameConstants::Chunk::Size;

			const int Index = UChunkData::GetIndex(X, Y);
			Heights[Index] = GetTerrainHeight(WorldX, WorldY);
		}
	}

	// Smooth terrain to reduce spikes and sharp edges
	SmoothTerrain(Heights);

	// Apply erosion simulation for more realistic terrain
	ApplyErosion(Heights, Position.X, Position.Y);

	// Additional smoothing pass after erosion
	SmoothTerrain(Heights);

	// Second pass: Build chunk columns with biome-based materials
	for (int X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);
			auto& Column = OutColumns[Index];

			const float WorldX = X + Position.X * GameConstants::Chunk::Size;
			const float WorldY = Y + Position.Y * GameConstants::Chunk::Size;

			const int TerrainHeight = FMath::Clamp(FMath::RoundToInt(Heights[Index]), 1, GameConstants::Chunk::Height - 1);

			// Get temperature and precipitation for biome determination
			const float Temperature = GetTemperature(WorldX, WorldY, TerrainHeight);
			const float Precipitation = GetPrecipitation(WorldX, WorldY);
			const EBiomeType Biome = GetBiome(Temperature, Precipitation);

			// Build column from bottom up
			TArray<FPiece> TempPieces;
			int CurrentHeight = 0;

			while (CurrentHeight < TerrainHeight)
			{
				// Determine material based on biome system
				EMaterial Material = GetMaterialForBiome(CurrentHeight, Biome, Temperature, Precipitation, CurrentHeight < SeaLevel);

				// Count consecutive same materials
				int MaterialHeight = 1;
				while (CurrentHeight + MaterialHeight < TerrainHeight)
				{
					EMaterial NextMaterial = GetMaterialForBiome(CurrentHeight + MaterialHeight, Biome, Temperature, Precipitation, (CurrentHeight + MaterialHeight) < SeaLevel);
					if (NextMaterial != Material)
					{
						break;
					}
					MaterialHeight++;
				}

				TempPieces.Emplace(Material, MaterialHeight);
				CurrentHeight += MaterialHeight;
			}

			// Add water if terrain is below sea level
			if (TerrainHeight < SeaLevel)
			{
				TempPieces.Emplace(EMaterial::Water, SeaLevel - TerrainHeight);
				CurrentHeight = SeaLevel;
			}

			// Add void above terrain
			if (CurrentHeight < GameConstants::Chunk::Height)
			{
				TempPieces.Emplace(EMaterial::Void, GameConstants::Chunk::Height - CurrentHeight);
			}

			// Optimize pieces by combining adjacent same materials
			for (const auto& Piece : TempPieces)
			{
				if (Column.Pieces.Num() > 0 && Column.Pieces.Last().MaterialId == Piece.MaterialId)
				{
					Column.Pieces.Last().Size += Piece.Size;
				}
				else
				{
					Column.Pieces.Add(Piece);
				}
			}
		}
	}

	// Generate instances
	OutEntities.Empty();

	if (InstanceTypes.Num() == 0 || MaxInstancesPerChunk <= 0)
	{
		return; // No instances to generate
	}

	// Use a deterministic random seed based on chunk position
	FRandomStream RandomStream(Position.X * 73856093 ^ Position.Y * 19349663 ^ 83492791);

	int32 InstancesGenerated = 0;

	// Try to place instances at random positions
	for (int32 Attempt = 0; Attempt < MaxInstancesPerChunk * 10 && InstancesGenerated < MaxInstancesPerChunk; ++Attempt)
	{
		// Random position within chunk
		const int32 X = RandomStream.RandRange(1, GameConstants::Chunk::Size - 2);
		const int32 Y = RandomStream.RandRange(1, GameConstants::Chunk::Size - 2);

		const int32 Index = UChunkData::GetIndex(X, Y);
		const int32 TerrainHeight = FMath::RoundToInt(Heights[Index]);

		// Skip underwater positions
		if (TerrainHeight < SeaLevel)
			continue;

		// Get biome info for this position
		const float WorldX = X + Position.X * GameConstants::Chunk::Size;
		const float WorldY = Y + Position.Y * GameConstants::Chunk::Size;
		const float Temperature = GetTemperature(WorldX, WorldY, TerrainHeight);
		const float Precipitation = GetPrecipitation(WorldX, WorldY);
		const EBiomeType Biome = GetBiome(Temperature, Precipitation);

		// Get the surface material at this position
		EMaterial SurfaceMaterial = GetMaterialForBiome(TerrainHeight, Biome, Temperature, Precipitation, false);

		// Try to find a suitable instance type for this position
		TArray<UInstanceTypeDataAsset*> ValidInstances;

		for (UInstanceTypeDataAsset* InstanceType : InstanceTypes)
		{
			if (!InstanceType)
				continue;

			// Check if this instance can spawn on this surface
			if (InstanceType->ValidSurfaces.Contains(SurfaceMaterial))
			{
				// Check spawn chance
				if (RandomStream.FRand() < InstanceType->SpawnChance)
				{
					ValidInstances.Add(InstanceType);
				}
			}
		}

		if (ValidInstances.Num() == 0)
			continue;

		// Select random instance from valid ones
		UInstanceTypeDataAsset* SelectedInstance = ValidInstances[RandomStream.RandRange(0, ValidInstances.Num() - 1)];

		// Check spacing requirements (ensure instances don't overlap)
		bool bCanPlace = true;
		const float RequiredSpacing = SelectedInstance->Radius * GameConstants::Scaling::XYWorldSize;
		const float RequiredSpacingLocal = SelectedInstance->Radius;

		for (const FEntityRecord& ExistingEntity : OutEntities)
		{
			FVector ExistingPos = ExistingEntity.Transform.GetLocation();
			float DistX = FMath::Abs((X * GameConstants::Scaling::XYWorldSize) - ExistingPos.X);
			float DistY = FMath::Abs((Y * GameConstants::Scaling::XYWorldSize) - ExistingPos.Y);

			if (DistX < RequiredSpacing && DistY < RequiredSpacing)
			{
				bCanPlace = false;
				break;
			}
		}

		if (!bCanPlace)
			continue;

		// Create entity record for this instance
		FEntityRecord NewEntity;

		// Random rotation around Z axis
		float RandomYaw = RandomStream.FRandRange(0.0f, 360.0f);

		// Random scale within defined range
		float RandomScale = RandomStream.FRandRange(SelectedInstance->MinScale, SelectedInstance->MaxScale);

		// Set transform (local to chunk)
		FVector LocalPosition(
			X * GameConstants::Scaling::XYWorldSize,
			Y * GameConstants::Scaling::XYWorldSize,
			TerrainHeight * GameConstants::Scaling::ZWorldSize
		);

		NewEntity.Transform = FTransform(
			FRotator(0.0f, RandomYaw, 0.0f),
			LocalPosition,
			FVector(RandomScale)
		);

		// Set the instance type
		NewEntity.InstanceTypeId = SelectedInstance->GetPrimaryAssetId();

		// Instance index will be assigned when added to HISM component
		NewEntity.InstanceIndex = INDEX_NONE;

		// Add to entities array
		const auto ArrayIndex = OutEntities.Add(NewEntity);
		OutEntities[ArrayIndex].ArrayIndex = ArrayIndex;
		
		InstancesGenerated++;
	}

	UE_LOG(LogTemp, Verbose, TEXT("Generated %d instances for chunk %s"), InstancesGenerated, *Position.ToString());
}

float UNoiseWorldGenerator::GetTerrainHeight(float WorldX, float WorldY) const
{
	// Continental shelf - base terrain shape
	float ContinentalValue = ContinentalNoise->GetNoise2D(WorldX, WorldY);
	ContinentalValue += LandBias;
	ContinentalValue = TerrainCurve(ContinentalValue) * TerrainAmplification;

	// Mountain variation using Worley and Ridge noise
	float MountainVariation = GetMountainVariation(WorldX, WorldY);

	// Mountain peaks with variation
	float PeaksValue = PeaksNoise->GetNoise2D(WorldX, WorldY);
	PeaksValue = FMath::Max(0.0f, PeaksValue - MountainThreshold);
	if (PeaksValue > 0.0f)
	{
		// Apply mountain variation for more interesting shapes
		PeaksValue = FMath::Pow(PeaksValue * 2.5f, 2.2f);
		PeaksValue *= (1.0f + MountainVariation * 0.4f);  // Add variation

		// Reduce spike frequency by smoothing extreme values
		PeaksValue = FMath::Min(PeaksValue, 2.0f);
	}

	// Erosion - more natural valley carving
	float ErosionValue = ErosionNoise->GetNoise2D(WorldX, WorldY);
	ErosionValue = FMath::Pow(FMath::Abs(ErosionValue), 1.5f) * FMath::Sign(ErosionValue);

	// Detail noise - reduced to prevent spikes
	float DetailValue = DetailNoise->GetNoise2D(WorldX, WorldY) * 0.08f;  // Reduced from 0.15f

	// Combine base height components
	float BaseHeight = ContinentalValue;

	// Add mountains with controlled influence
	if (PeaksValue > 0.0f)
	{
		// Smooth blend mountains into terrain
		float MountainBlend = SmoothStep(0.0f, 0.3f, PeaksValue);
		BaseHeight = Lerp(BaseHeight, BaseHeight + PeaksValue * 1.5f, MountainBlend);
	}

	// Apply erosion more naturally
	float ErosionFactor = FMath::Lerp(1.0f, FMath::Max(0.6f, 1.0f - FMath::Abs(ErosionValue) * 0.3f), ErosionStrength);
	if (BaseHeight < 0.5f)  // Only erode lower areas
	{
		BaseHeight *= ErosionFactor;
	}

	// Add detail
	BaseHeight += DetailValue;

	// Smooth height remapping to reduce sharp transitions
	float Height;

	if (BaseHeight < -0.2f)
	{
		// Ocean
		Height = Remap(BaseHeight, -1.5f, -0.2f, ValleyFloorHeight, SeaLevel);
	}
	else if (BaseHeight < 0.1f)
	{
		// Coastal transition
		float t = Remap(BaseHeight, -0.2f, 0.1f, 0.0f, 1.0f);
		Height = Lerp(SeaLevel, DefaultLandHeight - 10, SmoothStep(0.0f, 1.0f, t));
	}
	else if (BaseHeight < 0.5f)
	{
		// Plains and hills with smooth transitions
		float t = Remap(BaseHeight, 0.1f, 0.5f, 0.0f, 1.0f);
		Height = Lerp(DefaultLandHeight - 10, DefaultLandHeight + 40, t);
	}
	else if (BaseHeight < 1.0f)
	{
		// Foothills
		float t = Remap(BaseHeight, 0.5f, 1.0f, 0.0f, 1.0f);
		Height = Lerp(DefaultLandHeight + 40, DefaultLandHeight + 80, SmoothStep(0.0f, 1.0f, t));
	}
	else
	{
		// Mountains with controlled height
		float t = FMath::Min(Remap(BaseHeight, 1.0f, 2.5f, 0.0f, 1.0f), 1.0f);
		Height = Lerp(DefaultLandHeight + 80, MountainPeakHeight, FMath::Pow(t, 1.5f));
	}

	// Softer terracing for mountains
	if (Height > DefaultLandHeight + 60)
	{
		float TerraceStrength = 0.15f;  // Reduced from 0.3f
		float TerraceHeight = 12.0f;
		float Terrace = FMath::Fmod(Height, TerraceHeight) / TerraceHeight;
		Terrace = SmoothStep(0.3f, 0.7f, Terrace) * TerraceHeight;
		Height = FMath::Lerp(Height, FMath::FloorToFloat(Height / TerraceHeight) * TerraceHeight + Terrace, TerraceStrength);
	}

	return FMath::Clamp(Height, 1.0f, GameConstants::Chunk::Height - 1.0f);
}

float UNoiseWorldGenerator::GetTemperature(float WorldX, float WorldY, float Height) const
{
	// Base temperature from noise
	float Temperature = TemperatureNoise->GetNoise2D(WorldX, WorldY);

	// Latitude simulation (colder at extremes)
	float LatitudeFactor = FMath::Sin(WorldY * 0.001f);
	Temperature += LatitudeFactor * 0.3f;

	// Altitude affects temperature (colder at height)
	float AltitudeFactor = Remap(Height, SeaLevel, MountainPeakHeight, 0.0f, -0.8f);
	Temperature += AltitudeFactor;

	// Normalize to 0-1 range (0 = cold, 1 = hot)
	return FMath::Clamp((Temperature + 1.0f) * 0.5f, 0.0f, 1.0f);
}

float UNoiseWorldGenerator::GetPrecipitation(float WorldX, float WorldY) const
{
	// Base precipitation from noise
	float Precipitation = PrecipitationNoise->GetNoise2D(WorldX, WorldY);

	// Add some variation
	float Variation = PrecipitationNoise->GetNoise2D(WorldX * 2.0f, WorldY * 2.0f) * 0.3f;
	Precipitation += Variation;

	// Normalize to 0-1 range (0 = dry, 1 = wet)
	return FMath::Clamp((Precipitation + 1.0f) * 0.5f, 0.0f, 1.0f);
}

EBiomeType UNoiseWorldGenerator::GetBiome(float Temperature, float Precipitation) const
{
	// Whittaker biome classification
	// Temperature: 0 = Cold, 1 = Hot
	// Precipitation: 0 = Dry, 1 = Wet

	if (Temperature < 0.15f)
	{
		// Very cold
		if (Precipitation < 0.3f)
			return EBiomeType::Tundra;
		else
			return EBiomeType::Ice;
	}
	else if (Temperature < 0.35f)
	{
		// Cold
		if (Precipitation < 0.3f)
			return EBiomeType::Tundra;
		else if (Precipitation < 0.6f)
			return EBiomeType::Taiga;
		else
			return EBiomeType::Taiga;
	}
	else if (Temperature < 0.6f)
	{
		// Temperate
		if (Precipitation < 0.2f)
			return EBiomeType::Desert;
		else if (Precipitation < 0.5f)
			return EBiomeType::Grassland;
		else if (Precipitation < 0.8f)
			return EBiomeType::DeciduousForest;
		else
			return EBiomeType::TemperateRainforest;
	}
	else
	{
		// Hot/Tropical
		if (Precipitation < 0.3f)
			return EBiomeType::Desert;
		else if (Precipitation < 0.6f)
			return EBiomeType::Savanna;
		else
			return EBiomeType::TropicalRainforest;
	}
}

float UNoiseWorldGenerator::GetMountainVariation(float WorldX, float WorldY) const
{
	// Worley noise for cellular mountain patterns
	float WorleyValue = WorleyNoise->GetNoise2D(WorldX, WorldY);
	WorleyValue = 1.0f - FMath::Abs(WorleyValue);  // Invert for peaks at cell centers

	// Ridge noise for mountain ridges
	float RidgeValue = RidgeNoise->GetNoise2D(WorldX, WorldY);
	RidgeValue = 1.0f - FMath::Abs(RidgeValue);  // Create ridge effect

	// Combine both for varied mountain shapes
	float Variation = Lerp(WorleyValue, RidgeValue, 0.5f);

	// Add some detail
	float Detail = RidgeNoise->GetNoise2D(WorldX * 3.0f, WorldY * 3.0f) * 0.2f;
	Variation += Detail;

	return FMath::Clamp(Variation, -1.0f, 1.0f);
}

void UNoiseWorldGenerator::SmoothTerrain(TArray<float>& Heights) const
{
	// Gaussian blur to smooth terrain and reduce spikes
	TArray<float> SmoothedHeights = Heights;

	for (int X = 1; X < GameConstants::Chunk::Size - 1; ++X)
	{
		for (int Y = 1; Y < GameConstants::Chunk::Size - 1; ++Y)
		{
			const int Index = UChunkData::GetIndex(X, Y);

			// 3x3 Gaussian kernel
			float Sum = 0.0f;
			float Weight = 0.0f;

			for (int DX = -1; DX <= 1; ++DX)
			{
				for (int DY = -1; DY <= 1; ++DY)
				{
					const int NIndex = UChunkData::GetIndex(X + DX, Y + DY);
					float W = (DX == 0 && DY == 0) ? 4.0f : (DX == 0 || DY == 0) ? 2.0f : 1.0f;
					Sum += Heights[NIndex] * W;
					Weight += W;
				}
			}

			SmoothedHeights[Index] = Sum / Weight;

			// Limit change to prevent over-smoothing
			float Diff = SmoothedHeights[Index] - Heights[Index];
			SmoothedHeights[Index] = Heights[Index] + Diff * 0.5f;
		}
	}

	Heights = SmoothedHeights;
}

float UNoiseWorldGenerator::Lerp(float A, float B, float T)
{
	return A + (B - A) * FMath::Clamp(T, 0.0f, 1.0f);
}

void UNoiseWorldGenerator::ApplyErosion(TArray<float>& Heights, int32 ChunkX, int32 ChunkY) const
{
	// Gentle erosion simulation to preserve mountains while creating valleys
	const int Iterations = 2;  // Fewer iterations to preserve peaks
	const float ErosionRate = 0.04f;  // Gentler erosion
	const float DepositionRate = 0.02f;  // Less deposition

	for (int Iter = 0; Iter < Iterations; ++Iter)
	{
		TArray<float> NewHeights = Heights;

		for (int X = 1; X < GameConstants::Chunk::Size - 1; ++X)
		{
			for (int Y = 1; Y < GameConstants::Chunk::Size - 1; ++Y)
			{
				const int Index = UChunkData::GetIndex(X, Y);
				const float CurrentHeight = Heights[Index];

				// Calculate height differences with neighbors
				float TotalDiff = 0.0f;
				float MaxDiff = 0.0f;
				int LowestNeighbor = Index;

				// Check all 8 neighbors
				for (int DX = -1; DX <= 1; ++DX)
				{
					for (int DY = -1; DY <= 1; ++DY)
					{
						if (DX == 0 && DY == 0) continue;

						const int NX = X + DX;
						const int NY = Y + DY;
						const int NIndex = UChunkData::GetIndex(NX, NY);

						const float NeighborHeight = Heights[NIndex];
						const float Diff = CurrentHeight - NeighborHeight;

						if (Diff > 0)
						{
							TotalDiff += Diff;
							if (Diff > MaxDiff)
							{
								MaxDiff = Diff;
								LowestNeighbor = NIndex;
							}
						}
					}
				}

				// Apply erosion (but preserve mountains)
				if (TotalDiff > 0)
				{
					// Skip erosion for tall mountains to preserve peaks
					if (CurrentHeight > DefaultLandHeight + 60)
					{
						continue;  // Don't erode mountains
					}

					// Gentle erosion for lower areas
					float ErosionAmount = FMath::Min(MaxDiff * ErosionRate, CurrentHeight - ValleyFloorHeight);

					// Only apply erosion in low-lying areas
					if (CurrentHeight < DefaultLandHeight)
					{
						NewHeights[Index] -= ErosionAmount;

						// Deposit some material to the lowest neighbor
						if (LowestNeighbor != Index)
						{
							NewHeights[LowestNeighbor] += ErosionAmount * DepositionRate;
						}
					}
				}
			}
		}

		Heights = NewHeights;
	}
}

EMaterial UNoiseWorldGenerator::GetMaterialForBiome(float Height, EBiomeType Biome, float Temperature, float Precipitation, bool bIsUnderwater) const
{
	if (bIsUnderwater)
	{
		// Underwater materials
		if (Height < ValleyFloorHeight + 5)
		{
			return EMaterial::Stone;  // Deep ocean floor
		}
		else if (Height < SeaLevel - 10)
		{
			return EMaterial::Sand;  // Ocean floor
		}
		else
		{
			return EMaterial::Sand;  // Shallow water
		}
	}

	// Beach/coast
	if (Height < SeaLevel + 3)
	{
		return EMaterial::Sand;
	}

	// Temperature-based snow with gradual spread
	// Snow appears at different heights based on temperature
	float SnowLineBase = DefaultLandHeight + 100;  // Base snow line
	float SnowLine = SnowLineBase - (1.0f - Temperature) * 50.0f;  // Lower snow line in cold areas

	// Gradual snow transition
	if (Height > SnowLine)
	{
		float SnowFactor = Remap(Height, SnowLine, SnowLine + 30, 0.0f, 1.0f);
		SnowFactor *= (1.0f - Temperature);  // More snow in cold areas

		// Add some noise to the snow line for natural variation
		float NoiseOffset = DetailNoise->GetNoise2D(Height * 0.1f, Temperature * 100.0f) * 10.0f;
		SnowFactor = FMath::Clamp(SnowFactor + NoiseOffset * 0.1f, 0.0f, 1.0f);

		if (SnowFactor > 0.7f)
		{
			return EMaterial::Snow;
		}
		else if (SnowFactor > 0.4f && Height > DefaultLandHeight + 80)
		{
			// Transition zone - mix of stone and snow (represented as stone here)
			return EMaterial::Stone;
		}
	}

	// Biome-based materials
	switch (Biome)
	{
	case EBiomeType::Desert:
		if (Height < DefaultLandHeight + 20)
			return EMaterial::Sand;
		else
			return EMaterial::Stone;

	case EBiomeType::Savanna:
		if (Height < DefaultLandHeight + 15)
			return Precipitation < 0.4f ? EMaterial::Sand : EMaterial::Dirt;
		else if (Height < DefaultLandHeight + 40)
			return EMaterial::Dirt;
		else
			return EMaterial::Stone;

	case EBiomeType::TropicalRainforest:
		if (Height < DefaultLandHeight + 30)
			return EMaterial::Grass;
		else if (Height < DefaultLandHeight + 50)
			return EMaterial::Dirt;
		else
			return EMaterial::Stone;

	case EBiomeType::Grassland:
		if (Height < DefaultLandHeight + 40)
			return EMaterial::Grass;
		else if (Height < DefaultLandHeight + 60)
			return Precipitation < 0.3f ? EMaterial::Dirt : EMaterial::Grass;
		else
			return EMaterial::Stone;

	case EBiomeType::DeciduousForest:
		if (Height < DefaultLandHeight + 25)
			return EMaterial::Grass;
		else if (Height < DefaultLandHeight + 50)
			return EMaterial::Dirt;
		else
			return EMaterial::Stone;

	case EBiomeType::TemperateRainforest:
		if (Height < DefaultLandHeight + 20)
			return EMaterial::Grass;
		else if (Height < DefaultLandHeight + 45)
			return EMaterial::Dirt;
		else
			return EMaterial::Stone;

	case EBiomeType::Taiga:
		if (Height < DefaultLandHeight + 15)
			return Temperature < 0.25f ? EMaterial::Snow : EMaterial::Grass;
		else if (Height < DefaultLandHeight + 40)
			return EMaterial::Dirt;
		else
			return EMaterial::Stone;

	case EBiomeType::Tundra:
		if (Height < DefaultLandHeight + 10)
			return Temperature < 0.2f ? EMaterial::Snow : EMaterial::Dirt;
		else if (Height < DefaultLandHeight + 30)
			return EMaterial::Stone;
		else
			return Temperature < 0.3f ? EMaterial::Snow : EMaterial::Stone;

	case EBiomeType::Ice:
		return EMaterial::Snow;

	default:
		return EMaterial::Grass;
	}
}

float UNoiseWorldGenerator::Remap(float Value, float OldMin, float OldMax, float NewMin, float NewMax)
{
	return NewMin + (Value - OldMin) * (NewMax - NewMin) / (OldMax - OldMin);
}

float UNoiseWorldGenerator::SmoothStep(float Edge0, float Edge1, float X)
{
	float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

float UNoiseWorldGenerator::TerrainCurve(float Value)
{
	// Custom curve to favor land generation with dramatic mountains
	float Result = Value;

	if (Result > -0.2f)
	{
		// Shift most values to positive (land) territory
		Result = Result + 0.3f;  // Bias toward land

		if (Result > 0.0f)
		{
			// Amplify positive values for mountains
			Result = FMath::Pow(Result, 1.2f);

			// Create VERY dramatic peaks for mountain values
			if (Result > 0.4f)
			{
				Result = 0.4f + FMath::Pow((Result - 0.4f) * 3.0f, 2.5f);
			}
		}
	}
	else
	{
		// Keep some water areas but make them less common
		Result = -FMath::Pow(-Result, 2.0f) * 0.5f;  // Reduce ocean depth and frequency
	}

	// Add redistribution to create interesting features
	if (Result > 0.2f && Result < 0.4f)
	{
		// Create plateau effect for default land height
		Result = FMath::Lerp(Result, 0.3f, 0.7f);
	}

	return Result;
}
