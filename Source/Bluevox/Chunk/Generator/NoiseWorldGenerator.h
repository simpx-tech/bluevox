#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "Bluevox/Chunk/Data/Piece.h"
#include "NoiseWorldGenerator.generated.h"

class UFastNoiseWrapper;
class UInstanceTypeDataAsset;

UENUM()
enum class EBiome : uint8
{
    Ocean,
    Beach,
    Plains,
    Forest,
    Desert,
    Taiga,
    Tundra,
    Mountain
};

UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UNoiseWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

public:
	UNoiseWorldGenerator();

	virtual void PostLoad() override;

	// ---------- Tunables ----------
	// Seeds
	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	bool bFixedSeed = false;
	
	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 ContinentSeed = 0;

	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 MountainSeed = 1;

	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 DetailSeed = 2;

	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 TemperatureSeed = 3;

	UPROPERTY(EditAnywhere, Category = "Noise|Seeds")
	int32 MoistureSeed = 4;

	// Frequencies (applied to world XY block indices)
	UPROPERTY(EditAnywhere, Category = "Noise|Frequency")
	float ContinentFrequency = 0.002f; // very low frequency → big continents

	UPROPERTY(EditAnywhere, Category = "Noise|Frequency")
	float MountainFrequency = 0.008f;  // medium frequency → mountain bands

	UPROPERTY(EditAnywhere, Category = "Noise|Frequency")
	float DetailFrequency = 0.03f;     // high-frequency → small features

	UPROPERTY(EditAnywhere, Category = "Noise|Frequency")
	float TemperatureFrequency = 0.0015f;

	UPROPERTY(EditAnywhere, Category = "Noise|Frequency")
	float MoistureFrequency = 0.0020f;

	// Elevation parameters (in layers)
	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 SeaLevelLayers = 256;

	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 MaxOceanDepthLayers = 160;

	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 MinOceanDepthLayers = 8;

	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 BaseLandHeightLayers = 160; // typical hills above sea

	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 MountainExtraHeightLayers = 320; // extra on top when in mountain regions

	UPROPERTY(EditAnywhere, Category = "Terrain|Heights")
	int32 MaxDetailJitterLayers = 8; // micro variation from detail noise

	UPROPERTY(EditAnywhere, Category = "Terrain|Surface")
	int32 BeachDepthLayers = 4;

	UPROPERTY(EditAnywhere, Category = "Terrain|Surface")
	int32 TopSoilDepthLayers = 4; // thickness of grass/sand/snow cap

	// Additional dirt layers placed under topsoil in gentle slopes (not cliffs/mountains)
	UPROPERTY(EditAnywhere, Category = "Terrain|Surface")
	int32 SubSoilDepthLayers = 6;

	UPROPERTY(EditAnywhere, Category = "Terrain|Surface")
	int32 SnowLineAboveSeaLayers = 520; // approximate altitude when snow starts

	// Slope threshold (layers per block) above which we expose stone as a cliff face
	UPROPERTY(EditAnywhere, Category = "Terrain|Surface")
	int32 CliffSlopeThresholdLayersPerBlock = 8;

	// Instance spawning controls
	UPROPERTY(EditAnywhere, Category = "Instances|Spawn")
	bool bSpawnInstances = true;

	UPROPERTY(EditAnywhere, Category = "Instances|Spawn")
	float ForestSpawnMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Instances|Spawn")
	float PlainsSpawnMultiplier = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Instances|Spawn")
	float TaigaSpawnMultiplier = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Instances|Spawn")
	float DesertSpawnMultiplier = 0.2f;

	// Biome thresholds
	UPROPERTY(EditAnywhere, Category = "Biome")
	float DesertTempThreshold = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Biome")
	float ColdTempThreshold = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Biome")
	float DryMoistureThreshold = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Biome")
	float WetMoistureThreshold = 0.6f;

	// Optional instance types that biomes can use (not required)
	UPROPERTY(EditAnywhere, Category = "Instances")
	TArray<UInstanceTypeDataAsset*> InstanceTypes;

protected:
	// Cached noise wrappers
	UPROPERTY()
	UFastNoiseWrapper* ContinentNoise = nullptr;
	UPROPERTY()
	UFastNoiseWrapper* MountainNoise = nullptr;
	UPROPERTY()
	UFastNoiseWrapper* DetailNoise = nullptr;
	UPROPERTY()
	UFastNoiseWrapper* TempNoise = nullptr;
	UPROPERTY()
	UFastNoiseWrapper* MoistureNoise = nullptr;

	// Helpers
	static inline float RemapTo01(float v) { return 0.5f * (v + 1.0f); }
	static inline float Clamp01(float v) { return FMath::Clamp(v, 0.0f, 1.0f); }

	float Fractal2D(const UFastNoiseWrapper* Noise, float X, float Y, float BaseFreq, int32 Octaves, float Lacunarity, float Gain) const;
	float ComputeLandMask(float X, float Y) const;
	float ComputeMountainMask(float X, float Y) const;
	void ChooseBiome(float Temp01, float Moist01, bool bIsOcean, bool bIsMountain, int32 ElevationLayers, EBiome& OutBiome) const;
	void BuildColumn(int32 GroundHeightLayers, int32 SeaLevel, EBiome Biome, bool bIsCliff, bool bIsMountain, TArray<struct FPiece>& OutPieces) const;

public:
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns, TArray<struct FEntityRecord>& OutEntities) override;
};
