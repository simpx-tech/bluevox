// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ClientNetworkPacket.h"
#include "Bluevox/Chunk/Data/ChunkColumn.h"
#include "Bluevox/Chunk/Data/InstanceData.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "ChunkDataNetworkPacket.generated.h"

class UChunkData;

USTRUCT()
struct FChunkDataWithPosition
{
	GENERATED_BODY()

	FChunkDataWithPosition()
	{
	}

	FChunkDataWithPosition(const FChunkPosition& InPosition, const TArray<FChunkColumn>& InColumns,
		const TMap<FPrimaryAssetId, FInstanceCollection>& InInstances)
		: Position(InPosition), Columns(InColumns), Instances(InInstances)
	{
	}

	UPROPERTY()
	FChunkPosition Position;

	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	TMap<FPrimaryAssetId, FInstanceCollection> Instances;

	friend FArchive& operator<<(FArchive& Ar, FChunkDataWithPosition& Data)
	{
		Ar << Data.Position;
		Ar << Data.Columns;
		Ar << Data.Instances;
		return Ar;
	}
};

/**
 * 
 */
UCLASS()
class BLUEVOX_API UChunkDataNetworkPacket : public UClientNetworkPacket
{
	GENERATED_BODY()

	virtual void OnReceive(AGameManager* GameManager) override;

	virtual void Serialize(FArchive& Ar) override;

public:
	UPROPERTY()
	TArray<FChunkDataWithPosition> Data;
	
	UChunkDataNetworkPacket* Init(TArray<FChunkDataWithPosition>&& InData)
	{
		Data = MoveTemp(InData);
		return this;
	}
};
