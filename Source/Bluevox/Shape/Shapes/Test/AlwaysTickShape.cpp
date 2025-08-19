// Fill out your copyright notice in the Description page of Project Settings.


#include "AlwaysTickShape.h"

#include "Bluevox/Chunk/Position/LocalPosition.h"

void UAlwaysTickShape::GameTick(AGameManager* AGameManager, const FLocalPosition& Position,
                                UChunkData* WhereData, float DeltaTime) const
{
	UE_LOG(LogTemp, Log, TEXT("UAlwaysTickShape::GameTick called at position %s with delta time %f."), 
		*Position.ToString(), DeltaTime);
}
