// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseWorldGenerator.h"

#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "Bluevox/Chunk/Data/Piece.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Game/VoxelMaterial.h"
#include "Bluevox/Entity/EntityTypes.h"
#include "FastNoiseWrapper.h"
#include "Bluevox/Data/InstanceTypeDataAsset.h"

namespace
{
	static FORCEINLINE float Smooth01(float t)
	{
		return t * t * (3.0f - 2.0f * t);
	}
}

UNoiseWorldGenerator::UNoiseWorldGenerator()
{
	ContinentNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("ContinentalNoise"));
	MountainNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("MountainNoise"));
	DetailNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("DetailNoise"));
	TempNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("TempNoise"));
	MoistureNoise = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("MoistureNoise"));
}

void UNoiseWorldGenerator::PostLoad()
{
	Super::PostLoad();

	if (!bFixedSeed)
	{
		Seed = FMath::RandRange(0, 0xFFFFFFFF);
		ContinentSeed = FMath::RandRange(0, 0xFFFFFFFF);
		MountainSeed = FMath::RandRange(0, 0xFFFFFFFF);
		DetailSeed = FMath::RandRange(0, 0xFFFFFFFF);
		TemperatureSeed = FMath::RandRange(0, 0xFFFFFFFF);
		MoistureSeed = FMath::RandRange(0, 0xFFFFFFFF);

		// Setup noise types and seeds. Frequency will mostly be controlled by sampling scale.
		if (ContinentNoise)
		{
			ContinentNoise->SetupFastNoise(EFastNoise_NoiseType::Perlin, Seed + ContinentSeed, 1.0f);
		}
		if (MountainNoise)
		{
			MountainNoise->SetupFastNoise(EFastNoise_NoiseType::Perlin, Seed + MountainSeed, 1.0f);
		}
		if (DetailNoise)
		{
			DetailNoise->SetupFastNoise(EFastNoise_NoiseType::Perlin, Seed + DetailSeed, 1.0f);
		}
		if (TempNoise)
		{
			TempNoise->SetupFastNoise(EFastNoise_NoiseType::Perlin, Seed + TemperatureSeed, 1.0f);
		}
		if (MoistureNoise)
		{
			MoistureNoise->SetupFastNoise(EFastNoise_NoiseType::Perlin, Seed + MoistureSeed, 1.0f);
		}
	}
}

float UNoiseWorldGenerator::Fractal2D(const UFastNoiseWrapper* Noise, float X, float Y, float BaseFreq, int32 Octaves, float Lacunarity, float Gain) const
{
	if (!Noise)
	{
		return 0.0f;
	}
	float sum = 0.0f;
	float amp = 1.0f;
	float freq = BaseFreq;
	float ampSum = 0.0f;
	for (int32 o = 0; o < Octaves; ++o)
	{
		sum += Noise->GetNoise2D(X * freq, Y * freq) * amp;
		ampSum += amp;
		amp *= Gain;
		freq *= Lacunarity;
	}
	if (ampSum > KINDA_SMALL_NUMBER)
	{
		sum /= ampSum; // keep in roughly [-1,1]
	}
	return FMath::Clamp(sum, -1.0f, 1.0f);
}

float UNoiseWorldGenerator::ComputeLandMask(float X, float Y) const
{
	const float n = Fractal2D(ContinentNoise, X, Y, ContinentFrequency, 4, 2.0f, 0.5f); // [-1,1]
	float v = RemapTo01(n); // [0,1]
	// Sharpen coasts a bit
	v = Smooth01(v);
	return Clamp01(v);
}

float UNoiseWorldGenerator::ComputeMountainMask(float X, float Y) const
{
	// Ridged style: 1 - |noise|
	const float n = Fractal2D(MountainNoise, X, Y, MountainFrequency, 5, 2.0f, 0.5f); // [-1,1]
	float ridged = 1.0f - FMath::Abs(n);
	ridged = FMath::Pow(Clamp01(ridged), 2.0f);
	return ridged; // [0,1]
}

void UNoiseWorldGenerator::ChooseBiome(float Temp01, float Moist01, bool bIsOcean, bool bIsMountain, int32 ElevationLayers, EBiome& OutBiome) const
{
	if (bIsOcean)
	{
		OutBiome = EBiome::Ocean;
		return;
	}
	// Beach near sea level
	if (ElevationLayers - SeaLevelLayers <= BeachDepthLayers && ElevationLayers >= SeaLevelLayers)
	{
		OutBiome = EBiome::Beach;
		return;
	}
	if (bIsMountain)
	{
		OutBiome = EBiome::Mountain;
		return;
	}

	// Cold categories
	if (Temp01 < 0.2f)
	{
		OutBiome = EBiome::Tundra;
		return;
	}
	if (Temp01 < ColdTempThreshold)
	{
		OutBiome = EBiome::Taiga;
		return;
	}

	// Hot & dry desert
	if (Temp01 > DesertTempThreshold && Moist01 < DryMoistureThreshold)
	{
		OutBiome = EBiome::Desert;
		return;
	}

	// Wet → forest
	if (Moist01 > WetMoistureThreshold)
	{
		OutBiome = EBiome::Forest;
		return;
	}

	// Default
	OutBiome = EBiome::Plains;
}

void UNoiseWorldGenerator::BuildColumn(int32 GroundHeightLayers, int32 SeaLevel, EBiome Biome, bool bIsCliff, bool bIsMountain, TArray<FPiece>& OutPieces) const
{
	OutPieces.Reset();
	const int32 TotalH = GameConstants::Chunk::Height;
	GroundHeightLayers = FMath::Clamp(GroundHeightLayers, 1, TotalH - 1);
	SeaLevel = FMath::Clamp(SeaLevel, 0, TotalH - 1);

	// Decide surface materials with slope-aware rules
	EMaterial TopMat = EMaterial::Grass;
	EMaterial UnderMat = EMaterial::Dirt;
	int32 SurfaceThickness = 1;
	int32 UnderThickness = TopSoilDepthLayers;

	const bool bAboveSnow = GroundHeightLayers >= SeaLevel + SnowLineAboveSeaLayers;

	// Cliff override: expose rock faces regardless of biome (except deep ocean handled below)
	if (bIsCliff && Biome != EBiome::Ocean)
	{
		TopMat = bAboveSnow ? EMaterial::Snow : EMaterial::Stone;
		UnderMat = EMaterial::Stone;
		SurfaceThickness = 1;
		UnderThickness = FMath::Clamp(TopSoilDepthLayers - 1, 1, 8);
	}
	else
	{
		switch (Biome)
		{
			case EBiome::Ocean:
			{
				TopMat = (SeaLevel - GroundHeightLayers <= BeachDepthLayers + 6) ? EMaterial::Sand : EMaterial::Stone;
				UnderMat = (TopMat == EMaterial::Sand) ? EMaterial::Sand : EMaterial::Stone;
				SurfaceThickness = 1;
				UnderThickness = FMath::Clamp(TopSoilDepthLayers, 1, 12);
				break;
			}
			case EBiome::Beach:
			{
				TopMat = EMaterial::Sand;
				UnderMat = EMaterial::Sand;
				SurfaceThickness = 1;
				UnderThickness = TopSoilDepthLayers + 1;
				break;
			}
			case EBiome::Desert:
			{
				TopMat = EMaterial::Sand;
				UnderMat = EMaterial::Sand;
				SurfaceThickness = 1;
				UnderThickness = TopSoilDepthLayers + 2;
				break;
			}
			case EBiome::Plains:
			{
				TopMat = EMaterial::Grass;
				UnderMat = EMaterial::Dirt;
				UnderThickness = TopSoilDepthLayers + SubSoilDepthLayers; // make grasslands thicker
				break;
			}
			case EBiome::Forest:
			{
				TopMat = EMaterial::Grass;
				UnderMat = EMaterial::Dirt;
				UnderThickness = TopSoilDepthLayers + SubSoilDepthLayers;
				break;
			}
			case EBiome::Taiga:
			{
				TopMat = bAboveSnow ? EMaterial::Snow : EMaterial::Grass;
				UnderMat = EMaterial::Dirt;
				UnderThickness = TopSoilDepthLayers + (bAboveSnow ? 0 : SubSoilDepthLayers);
				break;
			}
			case EBiome::Tundra:
			{
				TopMat = EMaterial::Snow;
				UnderMat = EMaterial::Dirt;
				UnderThickness = TopSoilDepthLayers; // thin active layer over permafrost
				break;
			}
			case EBiome::Mountain:
			{
				if (bAboveSnow)
				{
					TopMat = EMaterial::Snow;
					UnderMat = EMaterial::Stone; // firmer base under snow
					SurfaceThickness = 1;
					UnderThickness = TopSoilDepthLayers + 1;
				}
				else
				{
					// Gentle mountain slopes get alpine grass instead of bare stone
					TopMat = EMaterial::Grass;
					UnderMat = EMaterial::Dirt;
					UnderThickness = TopSoilDepthLayers + SubSoilDepthLayers;
				}
				break;
			}
		}
	}

	// Build from bottom up
	int32 remainingToGround = GroundHeightLayers;

	// Deep stone base
	int32 baseStone = FMath::Max(0, remainingToGround - (SurfaceThickness + UnderThickness));
	if (baseStone > 0)
	{
		OutPieces.Emplace(EMaterial::Stone, static_cast<uint16>(baseStone));
		remainingToGround -= baseStone;
	}
	// Under layer
	int32 under = FMath::Min(UnderThickness, remainingToGround - SurfaceThickness);
	if (under > 0)
	{
		OutPieces.Emplace(UnderMat, static_cast<uint16>(under));
		remainingToGround -= under;
	}
	// Top surface
	int32 top = FMath::Min(SurfaceThickness, remainingToGround);
	if (top > 0)
	{
		OutPieces.Emplace(TopMat, static_cast<uint16>(top));
		remainingToGround -= top;
	}
	// Water if below sea level
	if (GroundHeightLayers < SeaLevel)
	{
		const int32 water = SeaLevel - GroundHeightLayers;
		if (water > 0)
		{
			OutPieces.Emplace(EMaterial::Water, static_cast<uint16>(water));
		}
	}
	// Void to top
	const int32 filled = FMath::Max(GroundHeightLayers, SeaLevel);
	const int32 voidH = TotalH - filled;
	if (voidH > 0)
	{
		OutPieces.Emplace(EMaterial::Void, static_cast<uint16>(voidH));
	}
}

void UNoiseWorldGenerator::GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns, TArray<FEntityRecord>& OutEntities)
{
	OutColumns.SetNum(GameConstants::Chunk::Size * GameConstants::Chunk::Size);
	OutEntities.Empty();

	const int32 ChunkSize = GameConstants::Chunk::Size;
	const int32 SeaLevel = SeaLevelLayers;

	// Deterministic RNG per chunk to keep spawns stable across sessions
	const int32 SeedMix = Seed ^ (Position.X * 92837111) ^ (Position.Y * 689287499);
	FRandomStream RNG(SeedMix);

	// Track placed spawns to enforce per-chunk spacing based on asset Radius (in blocks)
	struct FPlacedSpawn { FVector2D PosBlocks; float RadiusBlocks; };
	TArray<FPlacedSpawn> PlacedSpawns;
	PlacedSpawns.Reserve(64);

	for (int32 lx = 0; lx < ChunkSize; ++lx)
	{
		for (int32 ly = 0; ly < ChunkSize; ++ly)
		{
			const int32 gx = Position.X * ChunkSize + lx;
			const int32 gy = Position.Y * ChunkSize + ly;

			// Continents → oceans/land
			const float landMask = ComputeLandMask((float)gx, (float)gy); // [0,1]
			const float coast = landMask - 0.5f; // negative = ocean
			const bool bOcean = coast < 0.0f;

			// Mountains
			const float mMask = ComputeMountainMask((float)gx, (float)gy); // [0,1]

			// Detail jitter
			const float jitter = Fractal2D(DetailNoise, (float)gx, (float)gy, DetailFrequency, 3, 2.0f, 0.5f); // [-1,1]

			// Temperature & moisture
			float temp01 = RemapTo01(Fractal2D(TempNoise, (float)gx, (float)gy, TemperatureFrequency, 4, 2.0f, 0.5f));
			float moist01 = RemapTo01(Fractal2D(MoistureNoise, (float)gx, (float)gy, MoistureFrequency, 4, 2.0f, 0.5f));

			// Optional very subtle latitudinal gradient by world Y
			temp01 = Clamp01(temp01 - 0.15f * FMath::Abs(FMath::Sin((float)gy * 0.00005f)));

			int32 groundH = 0;
			if (bOcean)
			{
				const float oceanT = Clamp01(-coast * 2.0f); // 0 at shore, 1 deep ocean
				const float depth = FMath::Lerp((float)MinOceanDepthLayers, (float)MaxOceanDepthLayers, oceanT);
				const float depthJitter = MaxDetailJitterLayers * (jitter * 0.5f + 0.5f) - (MaxDetailJitterLayers * 0.5f);
				groundH = FMath::RoundToInt((float)SeaLevel - depth + depthJitter);
			}
			else
			{
				const float inland = Clamp01(coast * 2.0f); // 0 at shore, 1 deep inland
				float base = (float)SeaLevel + (float)BaseLandHeightLayers * inland;
				float mountainAdd = (float)MountainExtraHeightLayers * FMath::Pow(mMask * inland, 1.2f);
				float jitterLayers = (float)MaxDetailJitterLayers * jitter;
				groundH = FMath::RoundToInt(base + mountainAdd + jitterLayers);
			}

			groundH = FMath::Clamp(groundH, 1, GameConstants::Chunk::Height - 1);
			const bool bMountain = (!bOcean) && (mMask > 0.6f || groundH > SeaLevel + BaseLandHeightLayers + MountainExtraHeightLayers * 0.25f);

			EBiome biome = EBiome::Plains;
			ChooseBiome(temp01, moist01, bOcean, bMountain, groundH, biome);

			TArray<FPiece> pieces;
			pieces.Reserve(5);

			// Compute local slope to decide if this column is a cliff (expose stone) or gentle (grass/dirt)
			auto ComputeGroundH = [&](int32 X, int32 Y) -> int32
			{
				const float landM = ComputeLandMask((float)X, (float)Y);
				const float cst = landM - 0.5f;
				const bool ocean = cst < 0.0f;
				const float mM = ComputeMountainMask((float)X, (float)Y);
				const float j = Fractal2D(DetailNoise, (float)X, (float)Y, DetailFrequency, 3, 2.0f, 0.5f);
				int32 h = 0;
				if (ocean)
				{
					const float oceanT = Clamp01(-cst * 2.0f);
					const float depth = FMath::Lerp((float)MinOceanDepthLayers, (float)MaxOceanDepthLayers, oceanT);
					const float depthJitter = MaxDetailJitterLayers * (j * 0.5f + 0.5f) - (MaxDetailJitterLayers * 0.5f);
					h = FMath::RoundToInt((float)SeaLevel - depth + depthJitter);
				}
				else
				{
					const float inland = Clamp01(cst * 2.0f);
					float base = (float)SeaLevel + (float)BaseLandHeightLayers * inland;
					float mountainAdd = (float)MountainExtraHeightLayers * FMath::Pow(mM * inland, 1.2f);
					float jitterLayers = (float)MaxDetailJitterLayers * j;
					h = FMath::RoundToInt(base + mountainAdd + jitterLayers);
				}
				return FMath::Clamp(h, 1, GameConstants::Chunk::Height - 1);
			};

			const int32 hx_p = ComputeGroundH(gx + 1, gy);
			const int32 hx_m = ComputeGroundH(gx - 1, gy);
			const int32 hy_p = ComputeGroundH(gx, gy + 1);
			const int32 hy_m = ComputeGroundH(gx, gy - 1);
			const float dHdx = 0.5f * float(hx_p - hx_m);
			const float dHdy = 0.5f * float(hy_p - hy_m);
			const float slope = FMath::Sqrt(dHdx * dHdx + dHdy * dHdy);
			const bool bIsCliff = (!bOcean) && (slope >= (float)CliffSlopeThresholdLayersPerBlock);

			BuildColumn(groundH, SeaLevel, biome, bIsCliff, bMountain, pieces);

			const int32 idx = UChunkData::GetIndex(lx, ly);
			OutColumns[idx] = FChunkColumn{ MoveTemp(pieces) };

			// Instance spawning per column
			if (bSpawnInstances && InstanceTypes.Num() > 0)
			{
				// Determine surface material (last piece that is not Void/Water)
				EMaterial SurfaceMat = EMaterial::Void;
				const TArray<FPiece>& ColPieces = OutColumns[idx].Pieces;
				for (const FPiece& P : ColPieces)
				{
					if (P.MaterialId != EMaterial::Void && P.MaterialId != EMaterial::Water)
					{
						SurfaceMat = P.MaterialId;
					}
				}

				// Skip if underwater (ground below sea means first space above ground is Water)
				const bool bUnderWater = groundH < SeaLevel;
				if (!bUnderWater)
				{
					// Biome multiplier
					auto GetBiomeMultiplier = [&](EBiome B) -> float
					{
						switch (B)
						{
							case EBiome::Forest: return ForestSpawnMultiplier;
							case EBiome::Plains: return PlainsSpawnMultiplier;
							case EBiome::Taiga: return TaigaSpawnMultiplier;
							case EBiome::Desert: return DesertSpawnMultiplier;
							default: return 1.0f;
						}
					};
					const float BiomeMul = GetBiomeMultiplier(biome);

					// Attempt a single spawn per column at most
					// Shuffle a copy of InstanceTypes deterministically for variability
					TArray<UInstanceTypeDataAsset*> Types = InstanceTypes;
					for (int32 i = 0; i < Types.Num(); ++i)
					{
						int32 j = RNG.RandRange(i, Types.Num() - 1);
						Types.Swap(i, j);
					}

					for (UInstanceTypeDataAsset* Type : Types)
					{
						if (!Type) continue;
						// Surface filter
						if (Type->ValidSurfaces.Num() > 0 && !Type->ValidSurfaces.Contains(SurfaceMat))
						{
							continue;
						}
						// Chance
						const float EffChance = FMath::Clamp(Type->SpawnChance * BiomeMul, 0.0f, 1.0f);
						if (RNG.FRand() > EffChance)
						{
							continue;
						}

						// Spacing and clearance
						const float RadiusBlocks = FMath::Max(0.0f, Type->Radius);
						const float RequiredVoid = (float)FMath::Max(0, Type->Height);
						// Ensure enough void above ground (ignore if trivially satisfied)
						const int32 TotalH = GameConstants::Chunk::Height;
						const int32 VoidAbove = TotalH - groundH;
						if (VoidAbove < RequiredVoid)
						{
							continue;
						}

						// Enforce distance from prior placed spawns in this chunk
						const FVector2D Candidate((float)gx + 0.5f, (float)gy + 0.5f);
						bool bTooClose = false;
						for (const FPlacedSpawn& PS : PlacedSpawns)
						{
							const float Dist = FVector2D::Distance(Candidate, PS.PosBlocks);
							if (Dist < (RadiusBlocks + PS.RadiusBlocks))
							{
								bTooClose = true;
								break;
							}
						}
						if (bTooClose)
						{
							continue;
						}

						// Passed: create entity record
						const float XY = GameConstants::Scaling::XYWorldSize;
						const float ZS = GameConstants::Scaling::ZWorldSize;
						const float Scale = RNG.FRandRange(Type->MinScale, Type->MaxScale);
						const float Yaw = RNG.FRandRange(0.0f, 360.0f);

						FEntityRecord Rec;
						Rec.Transform = FTransform(
							FRotator(0.0f, Yaw, 0.0f),
							FVector((lx + 0.5f) * XY, (ly + 0.5f) * XY, (float)groundH * ZS),
							FVector(Scale)
						);
						Rec.InstanceTypeId = Type->GetPrimaryAssetId();
						OutEntities.Add(MoveTemp(Rec));

						PlacedSpawns.Add({ Candidate, RadiusBlocks * Scale });
						break; // one spawn max per column
					}
				}
			}
		}
	}
}
