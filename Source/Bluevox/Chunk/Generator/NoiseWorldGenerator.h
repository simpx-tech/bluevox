// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "NoiseWorldGenerator.generated.h"

class UFastNoiseWrapper;

// Whittaker biome classification
UENUM()
enum class EBiomeType : uint8
{
	Desert,
	Savanna,
	TropicalRainforest,
	Grassland,
	DeciduousForest,
	TemperateRainforest,
	Taiga,
	Tundra,
	Ice
};

/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UNoiseWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

	UNoiseWorldGenerator();

	// Multiple noise layers for terrain generation
	UPROPERTY()
	UFastNoiseWrapper* ContinentalNoise; // Large scale continent shapes

	UPROPERTY()
	UFastNoiseWrapper* ErosionNoise; // Erosion simulation

	UPROPERTY()
	UFastNoiseWrapper* PeaksNoise; // Mountain peaks

	UPROPERTY()
	UFastNoiseWrapper* DetailNoise; // Small scale details

	UPROPERTY()
	UFastNoiseWrapper* TemperatureNoise; // Temperature distribution

	UPROPERTY()
	UFastNoiseWrapper* PrecipitationNoise; // Precipitation/moisture distribution

	UPROPERTY()
	UFastNoiseWrapper* WorleyNoise; // Worley noise for mountain variation

	UPROPERTY()
	UFastNoiseWrapper* RidgeNoise; // Ridge noise for mountain features

	// Terrain configuration
	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "30", ClampMax = "100"))
	int32 SeaLevel = 60;  // Lower sea level for more land (15 meters)

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "200", ClampMax = "250"))
	int32 MountainPeakHeight = 245;  // Mountains can reach 245 blocks (61.25 meters)

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "10", ClampMax = "50"))
	int32 ValleyFloorHeight = 20;  // Valleys go down to 20 blocks (5 meters)

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "80", ClampMax = "140"))
	int32 DefaultLandHeight = 110;  // Default land height above sea level

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "0.001", ClampMax = "0.01"))
	float TerrainScale = 0.0015f;  // Larger features for more dramatic terrain

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "0.2", ClampMax = "0.7"))
	float MountainThreshold = 0.35f;  // Much lower threshold for frequent mountains

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "0.1", ClampMax = "0.6"))
	float ErosionStrength = 0.25f;  // Moderate erosion to preserve mountains

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float TerrainAmplification = 1.8f;  // Higher amplification for dramatic features

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "-0.5", ClampMax = "0.5"))
	float LandBias = 0.3f;  // Positive bias to favor land over water

	// Instance types to generate
	UPROPERTY(EditAnywhere, Category = "Instances", meta = (AllowAbstract = "false"))
	TArray<TSoftObjectPtr<class UInstanceTypeDataAsset>> InstanceTypesToGenerate;

public:
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;

	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
	                           TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const override;

private:
	// Terrain generation helpers
	float GetTerrainHeight(float WorldX, float WorldY) const;
	float GetTemperature(float WorldX, float WorldY, float Height) const;
	float GetPrecipitation(float WorldX, float WorldY) const;
	EBiomeType GetBiome(float Temperature, float Precipitation) const;
	void ApplyErosion(TArray<float>& Heights, int32 ChunkX, int32 ChunkY) const;
	void SmoothTerrain(TArray<float>& Heights) const;
	EMaterial GetMaterialForBiome(float Height, EBiomeType Biome, float Temperature, float Precipitation, bool bIsUnderwater) const;
	float GetMountainVariation(float WorldX, float WorldY) const;

	// Utility functions for terrain shaping
	static float Remap(float Value, float OldMin, float OldMax, float NewMin, float NewMax);
	static float SmoothStep(float Edge0, float Edge1, float X);
	static float TerrainCurve(float Value);
	static float Lerp(float A, float B, float T);

	void GenerateInstances(const FChunkPosition& Position, const TArray<FChunkColumn>& Columns,
	                      TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const;

	void GenerateInstancesOfType(class UInstanceTypeDataAsset* Asset, const FChunkPosition& Position,
	                             const TArray<FChunkColumn>& Columns, TArray<FInstanceData>& OutInstances) const;
};
