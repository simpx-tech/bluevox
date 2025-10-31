#pragma once

#include "CoreMinimal.h"
#include "ClientNetworkPacket.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Engine/AssetManager.h"
#include "EntityDestroyedInChunkPacket.generated.h"

UCLASS()
class BLUEVOX_API UEntityDestroyedInChunkPacket : public UClientNetworkPacket
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FChunkPosition ChunkPosition;

	UPROPERTY()
	FPrimaryAssetId InstanceTypeId;

	// Local-to-chunk transform for re-adding the instance on clients
	UPROPERTY()
	FTransform LocalTransform = FTransform::Identity;

	UEntityDestroyedInChunkPacket* Init(const FChunkPosition& InChunkPos, const FPrimaryAssetId& InTypeId, const FTransform& InLocal)
	{
		ChunkPosition = InChunkPos;
		InstanceTypeId = InTypeId;
		LocalTransform = InLocal;
		return this;
	}

	virtual void OnReceive(AGameManager* GameManager) override;
	virtual void Serialize(FArchive& Ar) override;
};