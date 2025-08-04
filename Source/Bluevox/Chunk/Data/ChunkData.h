// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "UObject/Object.h"
#include "ChunkData.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UChunkData : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	int32 Changes = 0;

	FRWLock Lock;
	
	virtual void Serialize(FArchive& Ar) override;

	// DEV get xyz
	// DEV set xyz
};
