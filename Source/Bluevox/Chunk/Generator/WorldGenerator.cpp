// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"

UWorldGenerator* UWorldGenerator::Init(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	return this;
}

void UWorldGenerator::GenerateChunk(const FLocalChunkPosition& Position,
                                    UChunkData* OutChunkData) const
{
}
