// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "Bluevox/Chunk/Position/LocalColumnPosition.h"
#include "Bluevox/Game/GameRules.h"
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
	UChunkData* Init(TArray<FChunkColumn>&& InColumns)
	{
		Columns = MoveTemp(InColumns);
		return this;
	}

	virtual void BeginDestroy() override;
	
	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	int32 Changes = 0;

	FRWLock Lock;
	
	virtual void Serialize(FArchive& Ar) override;

	void SerializeForSave(FArchive& Ar)
	{
		Ar << Columns;
	}

	static int32 GetIndex(const int32 X, const int32 Y)
	{
		return X + Y * GameRules::Chunk::Size;
	}
	
	static int32 GetIndex(const FLocalColumnPosition ColumnPosition)
	{
		return ColumnPosition.X + ColumnPosition.Y * GameRules::Chunk::Size;
	}
	
	FChunkColumn& GetColumn(const FLocalColumnPosition ColumnPosition)
	{
		return Columns[GetIndex(ColumnPosition)];
	}

	FPiece GetPieceCopy(const int32 X, const int32 Y, const int32 Z) const;

	// DEV if set to a border chunk, also schedule re-render on the neighbor chunks
	void SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece);
};
