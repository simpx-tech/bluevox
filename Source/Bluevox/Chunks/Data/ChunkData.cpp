// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkData.h"

void UChunkData::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	Ar << Columns;
}
