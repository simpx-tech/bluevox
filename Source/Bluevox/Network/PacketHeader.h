#pragma once

#include "CoreMinimal.h"
#include "PacketHeader.generated.h"

class UNetworkPacket;

USTRUCT(BlueprintType)
struct FPacketHeader
{
	GENERATED_BODY()

	FPacketHeader()
	{
	}

	explicit FPacketHeader(const uint32 InPacketId, const int32 InTotalSize = 0, const TSubclassOf<UNetworkPacket>& InPacketType = nullptr, const bool bIsLocal = false)
		:  PacketId(InPacketId), TotalSize(InTotalSize), bLocalPacket(bIsLocal), PacketType(InPacketType)
	{
	}

	UPROPERTY()
	uint32 PacketId = 0;

	UPROPERTY()
	int32 TotalSize = 0;

	UPROPERTY()
	bool bLocalPacket = false;

	UPROPERTY()
	TSubclassOf<UNetworkPacket> PacketType;
};
