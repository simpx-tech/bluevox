// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkPacket.h"
#include "Bluevox/Chunk/Data/Piece.h"
#include "Bluevox/Chunk/Position/GlobalPosition.h"
#include "UpdateChunkNetworkPacket.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UUpdateChunkNetworkPacket : public UNetworkPacket
{
	GENERATED_BODY()

	FGlobalPosition Position;
	
	FPiece Piece;
	
public:
	UUpdateChunkNetworkPacket* Init(const FGlobalPosition& InPosition, const FPiece& InPiece)
	{
		Position = InPosition;
		Piece = InPiece;
		return this;
	}
	
	virtual void Serialize(FArchive& Ar) override;

	virtual void OnReceive(AGameManager* GameManager) override;
};
