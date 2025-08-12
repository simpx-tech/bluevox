// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkDataNetworkPacket.h"

#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMapTaskManager.h"
#include "Bluevox/Game/GameManager.h"

void UChunkDataNetworkPacket::OnReceive(AGameManager* GameManager)
{
	UE_LOG(LogChunk, Verbose, TEXT("Received chunk data packet with %d chunks"), Data.Num());
	const auto VirtualMapTaskManager = GameManager->VirtualMap->TaskManager;
	const auto ChunkRegistry = GameManager->ChunkRegistry;

	for (auto& DataWithPosition : Data)
	{
		VirtualMapTaskManager->WaitingToBeSent.Remove(DataWithPosition.Position);
		ChunkRegistry->Th_RegisterChunk(DataWithPosition.Position, NewObject<UChunkData>()->Init(MoveTemp(DataWithPosition.Columns)));
	}
}

void UChunkDataNetworkPacket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Ar << Data;
	}
}

