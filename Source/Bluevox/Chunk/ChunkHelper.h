// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Utils/Face.h"
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
	static void GetBorderChunks(const FChunkPosition& GlobalPosition, int32 Distance, TSet<FChunkPosition>& OutPositions);
	
	static void GetChunksAroundLoadAndLive(const FChunkPosition& GlobalPosition, int32 Distance, TSet<FChunkPosition>& OutLoad, TSet<
	                                       FChunkPosition>& OutLive);

	static void GetChunksAround(const FChunkPosition& GlobalPosition, int32 Distance, TSet<FChunkPosition>& OutPositions);

	static int32 GetDistance(const FChunkPosition& A, const FChunkPosition& B);
};
