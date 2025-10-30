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

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const
{
	TMap<FPrimaryAssetId, FInstanceCollection> DummyInstances;
	GenerateChunk(Position, OutColumns, DummyInstances);
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
                                         TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const
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

	// Generate instances after terrain generation
	GenerateInstances(Position, OutColumns, OutInstances);
}

UWorldGenerator* UNoiseWorldGenerator::Init(AGameManager* InGameManager)
{
	// Call parent Init
	Super::Init(InGameManager);

	UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::Init - Starting initialization with %d instance types to generate"),
		InstanceTypesToGenerate.Num());

	// If no instance types are configured, try to add default ones
	if (InstanceTypesToGenerate.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::Init - No instance types configured, adding default Instance_Tree"));

		// Try the standard path for Instance_Tree
		TSoftObjectPtr<UInstanceTypeDataAsset> TreeAsset(FSoftObjectPath(TEXT("/Game/Data/Instances/Instance_Tree.Instance_Tree")));
		InstanceTypesToGenerate.Add(TreeAsset);

		// Try loading it to verify
		UInstanceTypeDataAsset* LoadedAsset = TreeAsset.LoadSynchronous();
		if (LoadedAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::Init - Successfully added and verified Instance_Tree asset"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NoiseWorldGenerator::Init - Failed to load Instance_Tree asset, instance generation will not work!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::Init - Found %d instance types already configured"),
			InstanceTypesToGenerate.Num());
	}

	// Preload all instance assets on the main thread
	PreloadInstanceAssets();

	return this;
}


void UNoiseWorldGenerator::PreloadInstanceAssets()
{
	LoadedInstanceTypes.Empty();

	// Preload all instance type assets on the game thread
	for (const auto& AssetPtr : InstanceTypesToGenerate)
	{
		if (AssetPtr.IsValid())
		{
			UInstanceTypeDataAsset* Asset = AssetPtr.LoadSynchronous();
			if (Asset)
			{
				LoadedInstanceTypes.Add(Asset);
				UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator: Preloaded instance asset %s"),
					*Asset->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("NoiseWorldGenerator: Failed to preload instance asset"));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator: Preloaded %d instance type assets"),
		LoadedInstanceTypes.Num());
}

void UNoiseWorldGenerator::GenerateInstances(const FChunkPosition& Position, const TArray<FChunkColumn>& Columns,
                                             TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const
{
	OutInstances.Empty();

	// If LoadedInstanceTypes is empty (e.g., on background thread), load from InstanceTypesToGenerate
	if (LoadedInstanceTypes.Num() == 0 && InstanceTypesToGenerate.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::GenerateInstances - LoadedInstanceTypes is empty, loading %d instance types directly"),
			InstanceTypesToGenerate.Num());

		for (const auto& AssetPtr : InstanceTypesToGenerate)
		{
			UInstanceTypeDataAsset* Asset = AssetPtr.LoadSynchronous();
			if (!Asset)
			{
				UE_LOG(LogTemp, Error, TEXT("NoiseWorldGenerator::GenerateInstances - Failed to load instance asset"));
				continue;
			}

			FPrimaryAssetId AssetId = Asset->GetPrimaryAssetId();
			if (!AssetId.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("NoiseWorldGenerator: Asset %s has invalid PrimaryAssetId!"),
					*Asset->GetName());
				continue;
			}

			// Create collection for this instance type
			FInstanceCollection& Collection = OutInstances.FindOrAdd(AssetId);
			Collection.InstanceTypeId = AssetId;

			// Generate instances for this type
			GenerateInstancesOfType(Asset, Position, Columns, Collection.Instances);

			// Rebuild cache for efficient rendering
			Collection.RebuildTransformCache();

			UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::GenerateInstances - Generated %d instances of %s for chunk (%d,%d)"),
				Collection.Instances.Num(), *Asset->GetName(), Position.X, Position.Y);
		}
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("NoiseWorldGenerator::GenerateInstances - Generating for chunk (%d,%d) with %d loaded instance types"),
			Position.X, Position.Y, LoadedInstanceTypes.Num());

		// Process each preloaded instance type (thread-safe)
		for (UInstanceTypeDataAsset* Asset : LoadedInstanceTypes)
		{
			if (!Asset)
			{
				continue;
			}

			FPrimaryAssetId AssetId = Asset->GetPrimaryAssetId();
			if (!AssetId.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("NoiseWorldGenerator: Asset %s has invalid PrimaryAssetId!"),
					*Asset->GetName());
				continue;
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

	if (OutInstances.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::GenerateInstances - Total %d instance collections generated for chunk (%d,%d)"),
			OutInstances.Num(), Position.X, Position.Y);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator::GenerateInstances - No instances generated for chunk (%d,%d)"),
			Position.X, Position.Y);
	}
}

void UNoiseWorldGenerator::GenerateInstancesOfType(UInstanceTypeDataAsset* Asset, const FChunkPosition& Position,
                                                   const TArray<FChunkColumn>& Columns, TArray<FInstanceData>& OutInstances) const
{
	if (!Asset)
	{
		return;
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("NoiseWorldGenerator::GenerateInstancesOfType - Generating %s for chunk (%d,%d)"),
		*Asset->GetName(), Position.X, Position.Y);

	// Use position-based seed for deterministic random generation
	const int32 Seed = Position.X * 73856093 + Position.Y * 19349663 + GetTypeHash(Asset->GetPrimaryAssetId());
	FRandomStream RandomStream(Seed);

	// Apply sensible defaults if not configured
	const float SpawnChance = Asset->SpawnChance > 0 ? Asset->SpawnChance : 0.05f;  // 5% default spawn chance
	const float MinSpacing = Asset->MinSpacing > 0 ? Asset->MinSpacing : 3.0f;      // 3 units minimum spacing
	const int32 RequiredVoidSpace = Asset->RequiredVoidSpace > 0 ? Asset->RequiredVoidSpace : 5; // 5 blocks void space
	const float MinScale = Asset->MinScale > 0 ? Asset->MinScale : 0.8f;
	const float MaxScale = Asset->MaxScale > 0 ? Asset->MaxScale : 1.2f;

	UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator: Generating %s with SpawnChance=%.2f, MinSpacing=%.1f, RequiredVoidSpace=%d"),
		*Asset->GetName(), SpawnChance, MinSpacing, RequiredVoidSpace);

	// Track placed instance positions for collision detection
	TArray<FVector2D> PlacedPositions;

	for (int32 X = 0; X < GameConstants::Chunk::Size; ++X)
	{
		for (int32 Y = 0; Y < GameConstants::Chunk::Size; ++Y)
		{
			// Random chance to spawn based on asset configuration
			if (RandomStream.FRand() > SpawnChance)
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
			if (SurfaceZ < 0)
			{
				continue;
			}

			// If no valid surfaces configured, default to spawning on grass and stone
			static bool bLoggedValidSurfaces = false;
			if (Asset->ValidSurfaces.Num() == 0)
			{
				// Log once per asset type
				if (!bLoggedValidSurfaces)
				{
					UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator: %s has no ValidSurfaces configured, defaulting to Grass and Stone"),
						*Asset->GetName());
					bLoggedValidSurfaces = true;
				}
				// Default to grass and stone surfaces
				if (SurfaceMaterial != EMaterial::Grass && SurfaceMaterial != EMaterial::Stone)
				{
					continue;
				}
			}
			else if (!Asset->ValidSurfaces.Contains(SurfaceMaterial))
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
			if (VoidAboveSurface < RequiredVoidSpace)
			{
				continue;
			}

			// Check collision with existing instances
			const FVector2D CurrentPos(X, Y);
			bool bTooClose = false;
			for (const FVector2D& PlacedPos : PlacedPositions)
			{
				const float DistanceSquared = FVector2D::DistSquared(CurrentPos, PlacedPos);
				if (DistanceSquared < MinSpacing * MinSpacing)
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
			const float ScaleVariation = RandomStream.FRandRange(MinScale, MaxScale);
			const FVector Scale(ScaleVariation, ScaleVariation, ScaleVariation);

			// Create instance with transform and asset reference
			FTransform Transform(Rotation, Location, Scale);
			// Create TSoftObjectPtr from asset path rather than loaded asset
			TSoftObjectPtr<UInstanceTypeDataAsset> AssetRef(FSoftObjectPath(Asset->GetPathName()));
			OutInstances.Add(FInstanceData(Transform, AssetRef));

			PlacedPositions.Add(CurrentPos);
		}
	}

	if (OutInstances.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NoiseWorldGenerator: Generated %d instances of %s for chunk (%d,%d)"),
			OutInstances.Num(), *Asset->GetName(), Position.X, Position.Y);
	}
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
