#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "Bluevox/Chunk/VirtualMap/ChunkState.h"
#include "RenderChunkPayload.generated.h"

USTRUCT(BlueprintType)
struct FRenderChunkPayload
{
	GENERATED_BODY()

	FRenderChunkPayload()
	{
	}

	UPROPERTY()
	EChunkState State = EChunkState::None;

	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	TArray<FChunkColumn> NorthColumns;

	UPROPERTY()
	TArray<FChunkColumn> SouthColumns;

	UPROPERTY()
	TArray<FChunkColumn> WestColumns;

	UPROPERTY()
	TArray<FChunkColumn> EastColumns;

	UPROPERTY()
	bool bDirty = false;
};
