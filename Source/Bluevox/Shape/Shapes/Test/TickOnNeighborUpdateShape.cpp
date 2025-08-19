// Fill out your copyright notice in the Description page of Project Settings.


#include "TickOnNeighborUpdateShape.h"

#include "Bluevox/Chunk/Position/LocalPosition.h"

void UTickOnNeighborUpdateShape::GameTick(AGameManager* AGameManager,
                                          const FLocalPosition& Position, UChunkData* WhereData, float DeltaTime) const
{
	UE_LOG(LogTemp, Log, TEXT("UTickOnNeighborUpdateShape::GameTick called for position %s."), *Position.ToString());
}
