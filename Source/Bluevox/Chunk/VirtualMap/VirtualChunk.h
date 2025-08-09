#pragma once

#include "CoreMinimal.h"
#include "ChunkState.h"
#include "VirtualChunk.generated.h"

class AMainController;

USTRUCT(BlueprintType)
struct FVirtualChunk
{
	GENERATED_BODY()
	
	FVirtualChunk()
	{
	}

	UPROPERTY()
	EChunkState State = EChunkState::None;

	UPROPERTY()
	uint16 LiveForRemotePlayersAmount = 0;

	UPROPERTY()
	uint16 FarForRemotePlayersAmount = 0;
};
