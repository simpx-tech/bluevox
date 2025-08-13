// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkData.h"

void UChunkData::BeginDestroy()
{
	UE_LOG(LogTemp, Verbose, TEXT("UChunkData::BeginDestroy called for ChunkData %p"), this);
	UObject::BeginDestroy();
}

void UChunkData::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	Ar << Columns;
}
