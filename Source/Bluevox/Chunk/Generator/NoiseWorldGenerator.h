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

	UPROPERTY(EditAnywhere, Category = "Instances")
	float TreeSpawnChance = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Instances")
	int32 TreeHeightInLayers = 8;

public:
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;

	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
	                           TMap<EInstanceType, FInstanceCollection>& OutInstances) const override;

private:
	void GenerateInstances(const FChunkPosition& Position, const TArray<FChunkColumn>& Columns, TMap<EInstanceType, FInstanceCollection>& OutInstances) const;
};
