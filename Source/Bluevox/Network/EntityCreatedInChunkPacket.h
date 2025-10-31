#pragma once

#include "CoreMinimal.h"
#include "ClientNetworkPacket.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Engine/AssetManager.h"
#include "EntityCreatedInChunkPacket.generated.h"

UCLASS()
class BLUEVOX_API UEntityCreatedInChunkPacket : public UClientNetworkPacket
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FChunkPosition ChunkPosition;

	UPROPERTY()
	FPrimaryAssetId InstanceTypeId;

	UPROPERTY()
	int32 InstanceIndex = INDEX_NONE;

	UEntityCreatedInChunkPacket* Init(const FChunkPosition& InChunkPos, const FPrimaryAssetId& InTypeId, int32 InIndex)
	{
		ChunkPosition = InChunkPos;
		InstanceTypeId = InTypeId;
		InstanceIndex = InIndex;
		return this;
	}

	virtual void OnReceive(AGameManager* GameManager) override;
	virtual void Serialize(FArchive& Ar) override;
};