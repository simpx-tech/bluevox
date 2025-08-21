// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "TestWorldGenerator.generated.h"

class UFastNoiseWrapper;
/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UTestWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

	UPROPERTY()
	UFastNoiseWrapper* Noise;

	void GenerateThreeColumns(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;

	void GenerateOneColumnTick(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;

	void GenerateNoise4X4(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;

	void GenerateTickAlwaysShape(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;

	void GenerateTickOnLoadShape(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;

	void GenerateTickOnNeighborUpdateShape(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;
	
public:
	UTestWorldGenerator();

	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;
};
