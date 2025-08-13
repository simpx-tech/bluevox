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
	uint16 LiveForCount = 0;

	UPROPERTY()
	uint16 LoadedForCount = 0;

	bool ShouldBeKeptAlive() const
	{
		return LiveForCount > 0 || LoadedForCount > 0;
	}

	void RecalculateState()
	{
		if (LiveForCount > 0)
		{
			State = bLiveLocal ? EChunkState::Live : EChunkState::RemoteLive;
		} else if (LoadedForCount > 0)
		{
			State = EChunkState::LoadOnly;
		} else
		{
			State = EChunkState::None;
		}
	}
	
	UPROPERTY()
	bool bLiveLocal = false;
};
