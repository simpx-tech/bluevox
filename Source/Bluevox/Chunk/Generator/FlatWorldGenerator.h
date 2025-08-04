// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "FlatWorldGenerator.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UFlatWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 GroundHeight = 64.0f;

	UPROPERTY(EditAnywhere)
	int32 ShapeId = 1;
	
	UPROPERTY(EditAnywhere)
	int32 MaterialId = 1;
	
	virtual void GenerateChunk(const FLocalChunkPosition& Position, UChunkData* OutChunkData) const override;

	virtual void Serialize(FArchive& Ar) override;
};
