#pragma once

#include "CoreMinimal.h"
#include "PacketHeader.h"
#include "PendingReceive.generated.h"

USTRUCT(BlueprintType)
struct FPendingReceive : public FPacketHeader
{
	GENERATED_BODY()

	FPendingReceive()
	{
	}

	explicit FPendingReceive(const FPacketHeader& Header)
		: FPacketHeader(Header)
	{
	}

	UPROPERTY()
	TArray<uint8> Data;

	// DEV request resend logic
	UPROPERTY()
	double LastPacketTime = 0;

	UPROPERTY()
	int32 MissingChunks = 0;

	TBitArray<> ReceivedChunks;
};
