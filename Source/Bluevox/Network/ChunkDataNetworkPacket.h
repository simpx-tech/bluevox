// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ClientNetworkPacket.h"
#include "ChunkDataNetworkPacket.generated.h"

class UChunkData;
/**
 * 
 */
UCLASS()
class BLUEVOX_API UChunkDataNetworkPacket : public UClientNetworkPacket
{
	GENERATED_BODY()

	UPROPERTY()
	UChunkData* ChunkData = nullptr;

	virtual void OnReceive(AGameManager* GameManager) override;

	virtual void Serialize(FArchive& Ar) override;

public:
	UChunkDataNetworkPacket* Init(UChunkData* InData);
};
