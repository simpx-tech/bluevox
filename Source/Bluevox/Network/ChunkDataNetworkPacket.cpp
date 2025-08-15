// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkDataNetworkPacket.h"

#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Chunk/VirtualMap/ChunkTaskManager.h"
#include "Bluevox/Game/GameManager.h"

void UChunkDataNetworkPacket::OnReceive(AGameManager* GameManager)
{
	UE_LOG(LogChunk, Verbose, TEXT("Received chunk data packet with %d chunks"), Data.Num());
	GameManager->ChunkTaskManager->HandleChunkDataNetworkPacket(this);
}

void UChunkDataNetworkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Ar << Data;
	}
}

