// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChunkHelper.generated.h"

struct FChunkPosition;
struct FGlobalPosition;
/**
 * 
 */
UCLASS()
class BLUEVOX_API UChunkHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void GetChunksAroundLiveAndFar(const FChunkPosition& GlobalPosition, int32 FarDistance, int32 LiveDistance, TSet<FChunkPosition>& OutFar, TSet<FChunkPosition>& OutLive);

	static void GetChunksAround(const FChunkPosition& GlobalPosition, int32 Distance, TSet<FChunkPosition>& OutPositions);
};
