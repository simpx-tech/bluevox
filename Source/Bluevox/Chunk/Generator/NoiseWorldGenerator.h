// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "NoiseWorldGenerator.generated.h"

class UFastNoiseWrapper;

/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UNoiseWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

	UNoiseWorldGenerator();

	UPROPERTY()
	UFastNoiseWrapper* Noise;

	UPROPERTY(EditAnywhere)
	int32 WaterLevel = 100;

	// Instance types to generate
	UPROPERTY(EditAnywhere, Category = "Instances", meta = (AllowAbstract = "false"))
	TArray<TSoftObjectPtr<class UInstanceTypeDataAsset>> InstanceTypesToGenerate;

public:
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;

	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
	                           TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const override;

private:
	void GenerateInstances(const FChunkPosition& Position, const TArray<FChunkColumn>& Columns,
	                      TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances) const;

	void GenerateInstancesOfType(class UInstanceTypeDataAsset* Asset, const FChunkPosition& Position,
	                             const TArray<FChunkColumn>& Columns, TArray<FInstanceData>& OutInstances) const;
};
