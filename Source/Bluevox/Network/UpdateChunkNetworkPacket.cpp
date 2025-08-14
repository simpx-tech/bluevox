// Fill out your copyright notice in the Description page of Project Settings.


#include "UpdateChunkNetworkPacket.h"

#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Game/GameManager.h"

void UUpdateChunkNetworkPacket::Serialize(FArchive& Ar)
{
	Ar << Position;
	Ar << Piece;
}

void UUpdateChunkNetworkPacket::OnReceive(AGameManager* GameManager)
{
	GameManager->ChunkRegistry->SetPiece(Position, Piece);
}
