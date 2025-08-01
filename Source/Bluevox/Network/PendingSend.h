#pragma once

#include "CoreMinimal.h"
#include "PacketHeader.h"
#include "PendingSend.generated.h"

USTRUCT(BlueprintType)
struct FPendingSend : public FPacketHeader
{
	GENERATED_BODY()

	FPendingSend()
	{
	}

	FPendingSend(const uint32 InPacketId, const TSubclassOf<UNetworkPacket>& PacketType, bool bIsLocalPacket, TArray<uint8>&& InData)
		: FPacketHeader(InPacketId, InData.Num(), PacketType, bIsLocalPacket), Data(MoveTemp(InData))
	{
	}

	explicit FPendingSend(const FPacketHeader& InHeader)
		: FPacketHeader(InHeader)
	{
	}
	
	UPROPERTY()
	TArray<uint8> Data;

	UPROPERTY()
	int32 Offset = 0;

	UPROPERTY()
	int32 LastOffset = 0;

	UPROPERTY()
	bool bSentAll = false;
};
