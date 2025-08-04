// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkDataNetworkPacket.h"

#include "Bluevox/Chunk/Data/ChunkData.h"

void UChunkDataNetworkPacket::OnReceive(AGameManager* GameManager)
{
	// DEV save to ChunkRegistry + Start spawn chunk process (?)
}

void UChunkDataNetworkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	ChunkData->Serialize(Ar);
}

UChunkDataNetworkPacket* UChunkDataNetworkPacket::Init(UChunkData* InData)
{
	ChunkData = InData;
	return this;
}
