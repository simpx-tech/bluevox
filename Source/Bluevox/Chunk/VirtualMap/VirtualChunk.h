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
	uint16 LiveForPlayersAmount = 0;
	
	UPROPERTY()
	EChunkState State = EChunkState::None;
};
