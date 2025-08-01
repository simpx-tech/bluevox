#pragma once

#include "CoreMinimal.h"
#include "PacketChunk.generated.h"

USTRUCT(BlueprintType)
struct FPacketChunk
{
	GENERATED_BODY()

	FPacketChunk()
	{
	}

	UPROPERTY()
	int32 PacketId = 0;

	UPROPERTY()
	int32 Index = 0;

	UPROPERTY()
	TArray<uint8> Data;
};
