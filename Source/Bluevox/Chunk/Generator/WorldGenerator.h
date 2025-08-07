// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "UObject/Object.h"
#include "WorldGenerator.generated.h"

struct FChunkPosition;
class AGameManager;
class UChunkData;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UWorldGenerator : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	AGameManager* GameManager = nullptr;
	
public:
	UWorldGenerator* Init(AGameManager* InGameManager);
	
	virtual void GenerateChunk(const FChunkPosition& Position, TArray<FChunkColumn>& OutColumns) const;
};
