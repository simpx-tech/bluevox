// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "Bluevox/Game/GameRules.h"
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
	FName ShapeName = GameRules::Constants::GShape_Layer;
	
	UPROPERTY(EditAnywhere)
	FName MaterialName = GameRules::Constants::GMaterial_Stone;
	
	virtual void GenerateChunk(const FLocalChunkPosition& Position, UChunkData* OutChunkData) const override;

	virtual void Serialize(FArchive& Ar) override;
};
