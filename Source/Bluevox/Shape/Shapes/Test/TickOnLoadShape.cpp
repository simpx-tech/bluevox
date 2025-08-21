// Fill out your copyright notice in the Description page of Project Settings.


#include "TickOnLoadShape.h"

#include "Bluevox/Chunk/Position/LocalPosition.h"

void UTickOnLoadShape::GameTick(AGameManager* AGameManager, const FLocalPosition& Position,
                                uint16 Height, UChunkData* WhereData, float DeltaTime) const
{
	UE_LOG(LogTemp, Log, TEXT("UTickOnLoadShape::GameTick called at position %s."), *Position.ToString());
}
