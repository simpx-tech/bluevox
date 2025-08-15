// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkData.h"

#include "Bluevox/Chunk/LogChunk.h"

void UChunkData::Serialize(FArchive& Ar)
{
	FReadScopeLock ReadLock(Lock);
	UObject::Serialize(Ar);
	Ar << Columns;
}

// TODO in future may have to consider potential caves
int32 UChunkData::GetFirstGapThatFits(const int32 X, const int32 Y, const int32 FitHeightInLayers)
{
	const auto Index = GetIndex(X, Y);
	if (!Columns.IsValidIndex(Index))
	{
		UE_LOG(LogChunk, Warning, TEXT("GetSurfacePosition: Invalid column index %d for %d,%d"), Index, X, Y);
		return GameRules::Chunk::Height;
	}

	const auto& Column = Columns[Index];
	if (Column.Pieces.Num() == 0)
	{
		return GameRules::Chunk::Height;
	}

	int32 CurZ = 0;
	for (const auto& Piece : Column.Pieces)
	{
		if (Piece.Id == 0 && Piece.Size >= FitHeightInLayers)
		{
			return CurZ;
		}
		
		CurZ += Piece.Size;
	}

	return CurZ;
}

bool UChunkData::DoesFit(const int32 X, const int32 Y, const int32 Z, const int32 FitHeight) const
{
	if (FitHeight <= 0) return false;
	if (Z < 0) return false;

	const int32 ColIndex = GetIndex(X, Y);
	if (!Columns.IsValidIndex(ColIndex))
	{
		UE_LOG(LogChunk, Warning, TEXT("DoesFit: Invalid column index %d for %d,%d"), ColIndex, X, Y);
		return false;
	}

	const auto& Column = Columns[ColIndex];

	int32 CurZ = 0;
	for (const auto& Piece : Column.Pieces)
	{
		const int32 PieceEnd = CurZ + Piece.Size;

		// Z is inside this piece
		if (Z < PieceEnd)
		{
			if (Piece.Id != GameRules::Constants::GShapeId_Void) return false;
			const int32 Remaining = PieceEnd - Z;
			return Remaining >= FitHeight;
		}
		CurZ = PieceEnd;
	}

	return Z + FitHeight <= GameRules::Chunk::Height;
}

FPiece UChunkData::Th_GetPieceCopy(const int32 X, const int32 Y, const int32 Z)
{
	FReadScopeLock ReadLock(Lock);
	
	const auto ColIndex = GetIndex(X, Y);
	if (!Columns.IsValidIndex(ColIndex))
	{
		UE_LOG(LogChunk, Warning, TEXT("GetPieceCopy: Invalid column index %d for %d,%d"), ColIndex, X, Y);
		return FPiece();
	}

	const auto& Column = Columns[ColIndex];
	int32 CurZ = 0;
	for (const auto& Piece : Column.Pieces)
	{
		if (CurZ + Piece.Size >= Z)
		{
			return Piece;
		}

		CurZ += Piece.Size;
	}

	UE_LOG(LogChunk, Warning, TEXT("GetPieceCopy: No piece found at %d,%d,%d"), X, Y, Z);
	return FPiece();
}

void UChunkData::Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece)
{
	FWriteScopeLock WriteLock(Lock);
	
	const auto ColIndex = GetIndex(X, Y);
	if (!Columns.IsValidIndex(ColIndex))
	{
		UE_LOG(LogChunk, Warning, TEXT("SetPiece: Invalid column index %d for %d,%d"), ColIndex, X, Y);
		return;
	}

	if (Piece.Size <= 0)
	{
		return;
	}

	auto& Column = Columns[ColIndex];
	
	const int32 NewStart = Z;
	const int32 NewEnd   = NewStart + Piece.Size;

	int32 CurZ = 0;
	int32 Idx  = 0;

	// Advance to the first piece that might overlap with the new piece
	while (Idx < Column.Pieces.Num() && CurZ + Column.Pieces[Idx].Size <= NewStart)
	{
		CurZ += Column.Pieces[Idx].Size;
		++Idx;
	}

	const int32 StartReplaceIdx = Idx;
	TArray<FPiece> Insert;

	if (Idx < Column.Pieces.Num())
	{
		const int32 AmountBeforeStart = FMath::Max<int32>(0, NewStart - CurZ);
		if (AmountBeforeStart > 0)
		{
			// keep the bottom of the first overlapped piece
			Insert.Emplace(Column.Pieces[Idx].Id, AmountBeforeStart);
		}
	}

	// Advance until the end of the new piece
	while (Idx < Column.Pieces.Num() &&
		   CurZ + Column.Pieces[Idx].Size < NewEnd)
	{
		CurZ += Column.Pieces[Idx].Size;
		++Idx;
	}

	Insert.Emplace(Piece.Id, Piece.Size);

	const int32 StopReplaceIdx = Idx;
	if (Idx < Column.Pieces.Num())
	{
		const int32 PieceEndZ = CurZ + Column.Pieces[Idx].Size;
		const int32 AmountAfterEnd = FMath::Max<int32>(0, PieceEndZ - static_cast<int32>(NewEnd));
		if (AmountAfterEnd > 0)
		{
			Insert.Emplace(Column.Pieces[Idx].Id, AmountAfterEnd);
		}
	}

	// Remove overlapped range [StartReplaceIdx - StopReplaceIdx] if any
	if (StartReplaceIdx < Column.Pieces.Num())
	{
		// If we ended past the end, remove through the last element
		const int32 InclusiveStop = StopReplaceIdx >= StartReplaceIdx && StopReplaceIdx < Column.Pieces.Num()
									? StopReplaceIdx
									: Column.Pieces.Num() - 1;
		const int32 NumToRemove = InclusiveStop >= StartReplaceIdx ? InclusiveStop - StartReplaceIdx + 1 : 0;
		if (NumToRemove > 0)
		{
			Column.Pieces.RemoveAt(StartReplaceIdx, NumToRemove);
		}
		// Insert at StartReplaceIdx
		Column.Pieces.Insert(Insert, StartReplaceIdx);
	}
	else
	{
		// Insert at the very end, nothing removed
		Column.Pieces.Append(Insert);
	}

	// Merge neighbors with the same id
	for (int32 i = 1; i < Column.Pieces.Num(); )
	{
		if (Column.Pieces[i - 1].Id == Column.Pieces[i].Id)
		{
			Column.Pieces[i - 1].Size += Column.Pieces[i].Size;
			Column.Pieces.RemoveAt(i);
		}
		else
		{
			++i;
		}
	}

	Changes++;
}
