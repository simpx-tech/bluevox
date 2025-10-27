// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Game/VoxelMaterial.h"
#include "FlatWorldGenerator.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UFlatWorldGenerator : public UWorldGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 GroundHeight = 64.0f;
	
	UPROPERTY(EditAnywhere)
	FName ShapeName = "";

	UPROPERTY(EditAnywhere)
	EMaterial Shape;
	
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const override;

	virtual void Serialize(FArchive& Ar) override;
};
