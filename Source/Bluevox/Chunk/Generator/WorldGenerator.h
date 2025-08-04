// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldGenerator.generated.h"

struct FLocalChunkPosition;
class UChunkData;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UWorldGenerator : public UObject
{
	GENERATED_BODY()

public:
	virtual void GenerateChunk(const FLocalChunkPosition& Position, UChunkData* OutChunkData) const;
};
