// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkData.h"

uint16 UChunkData::RegisterPalette(int32 ShapeId, int32 MaterialId)
{
	// DEV
}

void UChunkData::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	Ar << Palette;
	Ar << Columns;
}
