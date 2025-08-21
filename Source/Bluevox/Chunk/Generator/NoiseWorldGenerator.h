// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "Bluevox/Game/GameConstants.h"
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

public:
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;
};
