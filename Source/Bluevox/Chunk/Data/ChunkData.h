// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "Bluevox/Chunk/Position/LocalColumnPosition.h"
#include "Bluevox/Chunk/Position/LocalPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "UObject/Object.h"
#include "ChunkData.generated.h"

struct FGlobalPosition;
class AGameManager;
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
	
	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	int32 Changes = 0;

	FRWLock Lock;
	
	virtual void Serialize(FArchive& Ar) override;

	void SerializeForSave(FArchive& Ar)
	{
		FReadScopeLock ReadLock(Lock);
		int32 FileVersion = GameConstants::Chunk::File::FileVersion;
		
		Ar << FileVersion;
		Ar << Columns;
	}

	inline int32 GetFirstGapThatFits(const FGlobalPosition& GlobalPosition, const int32 FitHeightInLayers);
	
	int32 GetFirstGapThatFits(const int32 X, const int32 Y, const int32 FitHeightInLayers);

	bool DoesFit(const FGlobalPosition& GlobalPosition, const int32 FitHeightInLayers) const;
	
	bool DoesFit(const int32 X, const int32 Y, const int32 Z, const int32 FitHeightInLayers) const;

	static int32 GetIndex(const int32 X, const int32 Y)
	{
		return X + Y * GameConstants::Chunk::Size;
	}
	
	static int32 GetIndex(const FLocalColumnPosition ColumnPosition)
	{
		return ColumnPosition.X + ColumnPosition.Y * GameConstants::Chunk::Size;
	}
	
	FChunkColumn& GetColumn(const FLocalColumnPosition ColumnPosition)
	{
		return Columns[GetIndex(ColumnPosition)];
	}

	inline FPiece Th_GetPieceCopy(FLocalPosition LocalPosition);
	
	FPiece Th_GetPieceCopy(const int32 X, const int32 Y, const int32 Z);

	void Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece);
};
