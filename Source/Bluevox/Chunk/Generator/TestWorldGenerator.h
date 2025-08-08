// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "TestWorldGenerator.generated.h"

class UFastNoiseWrapper;
/**
 * 
 */
UCLASS()
class BLUEVOX_API UTestWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

	UPROPERTY()
	UFastNoiseWrapper* Noise;
	
public:
	UTestWorldGenerator();

	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;
};
