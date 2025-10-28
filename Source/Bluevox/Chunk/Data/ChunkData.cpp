// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkData.h"

#include "PieceWithStart.h"
#include "Bluevox/Chunk/LogChunk.h"
#include "Bluevox/Chunk/Position/GlobalPosition.h"
#include "Bluevox/Tick/TickManager.h"
#include "Bluevox/Entity/EntityFacade.h"
#include "Bluevox/Inventory/ItemWorldActor.h"

UChunkData* UChunkData::Init(AGameManager* InGameManager, const FChunkPosition InPosition,
	TArray<FChunkColumn>&& InColumns, TMap<FPrimaryAssetId, FInstanceCollection>&& InInstances)
{
	GameManager = InGameManager;
	Position = InPosition;
	Columns = MoveTemp(InColumns);
	InstanceCollections = MoveTemp(InInstances);

	// Build spatial index for fast lookups
	BuildSpatialIndex();

	GameManager->TickManager->RegisterUObjectTickable(this);

	return this;
}

void UChunkData::SerializeForSave(FArchive& Ar)
{
	FReadScopeLock ReadLock(Lock);
	int32 FileVersion = GameConstants::Chunk::File::FileVersion;

	Ar << FileVersion;
	Ar << Columns;

	// Serialize instance collections
	int32 NumCollections = InstanceCollections.Num();
	Ar << NumCollections;

	if (Ar.IsSaving())
	{
		for (auto& Pair : InstanceCollections)
		{
			FInstanceCollection& Collection = Pair.Value;
			Ar << Collection;
		}
	}
	else if (Ar.IsLoading())
	{
		InstanceCollections.Empty();
		for (int32 i = 0; i < NumCollections; ++i)
		{
			FInstanceCollection Collection;
			Ar << Collection;
			InstanceCollections.Add(Collection.InstanceTypeId, Collection);
		}
	}

	// Serialize world items
	TArray<FWorldItemData> WorldItems;

	if (Ar.IsSaving())
	{
		// Collect all valid world items
		for (auto& Pair : WorldItemGrid)
		{
			if (AItemWorldActor* ItemActor = Pair.Value.Get())
			{
				FWorldItemData ItemData(
					ItemActor->GetItemType(),
					ItemActor->GetStackAmount(),
					ItemActor->GetActorLocation(),
					ItemActor->GetActorRotation()
				);
				WorldItems.Add(ItemData);
			}
		}
	}

	int32 NumWorldItems = WorldItems.Num();
	Ar << NumWorldItems;

	if (Ar.IsSaving())
	{
		for (FWorldItemData& ItemData : WorldItems)
		{
			Ar << ItemData;
		}
	}
	else if (Ar.IsLoading())
	{
		WorldItems.SetNum(NumWorldItems);
		for (int32 i = 0; i < NumWorldItems; ++i)
		{
			Ar << WorldItems[i];
		}
		// Items will be spawned later when chunk is loaded
	}
}

void UChunkData::Serialize(FArchive& Ar)
{
	FReadScopeLock ReadLock(Lock);
	UObject::Serialize(Ar);
	Ar << Columns;

	// Serialize instance collections
	int32 NumCollections = InstanceCollections.Num();
	Ar << NumCollections;

	if (Ar.IsSaving())
	{
		for (auto& Pair : InstanceCollections)
		{
			FInstanceCollection& Collection = Pair.Value;
			Ar << Collection;
		}
	}
	else if (Ar.IsLoading())
	{
		InstanceCollections.Empty();
		for (int32 i = 0; i < NumCollections; ++i)
		{
			FInstanceCollection Collection;
			Ar << Collection;
			InstanceCollections.Add(Collection.InstanceTypeId, Collection);
		}
	}
}

int32 UChunkData::GetFirstGapThatFits(const FGlobalPosition& GlobalPosition,
	const int32 FitHeightInLayers)
{
	return GetFirstGapThatFits(GlobalPosition.X, GlobalPosition.Y, FitHeightInLayers);
}

// TODO in future may have to consider potential caves
int32 UChunkData::GetFirstGapThatFits(const int32 X, const int32 Y, const int32 FitHeightInLayers)
{
	const auto Index = GetIndex(X, Y);
	if (!Columns.IsValidIndex(Index))
	{
		UE_LOG(LogChunk, Warning, TEXT("GetSurfacePosition: Invalid column index %d for %d,%d"), Index, X, Y);
		return GameConstants::Chunk::Height;
	}

	const auto& Column = Columns[Index];
	if (Column.Pieces.Num() == 0)
	{
		return GameConstants::Chunk::Height;
	}

	int32 CurZ = 0;
	for (const auto& Piece : Column.Pieces)
	{
		if (Piece.MaterialId == EMaterial::Void && Piece.Size >= FitHeightInLayers)
		{
			return CurZ;
		}
		
		CurZ += Piece.Size;
	}

	return CurZ;
}

bool UChunkData::DoesFit(const FGlobalPosition& GlobalPosition, const int32 FitHeightInLayers) const
{
	return DoesFit(GlobalPosition.X, GlobalPosition.Y, GlobalPosition.Z, FitHeightInLayers);
}

bool UChunkData::DoesFit(const int32 X, const int32 Y, const int32 Z, const int32 FitHeightInLayers) const
{
	if (FitHeightInLayers <= 0) return false;
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
			if (Piece.MaterialId != EMaterial::Void) return false;
			const int32 Remaining = PieceEnd - Z;
			return Remaining >= FitHeightInLayers;
		}
		CurZ = PieceEnd;
	}

	return Z + FitHeightInLayers <= GameConstants::Chunk::Height;
}

FPieceWithStart UChunkData::Th_GetPieceCopy(const FLocalPosition LocalPosition)
{
	return Th_GetPieceCopy(LocalPosition.X, LocalPosition.Y, LocalPosition.Z);
}

FPieceWithStart UChunkData::Th_GetPieceCopy(const int32 X, const int32 Y, const int32 Z)
{
	FReadScopeLock ReadLock(Lock);
	
	const auto ColIndex = GetIndex(X, Y);
	if (!Columns.IsValidIndex(ColIndex))
	{
		UE_LOG(LogChunk, Warning, TEXT("GetPieceCopy: Invalid column index %d for %d,%d"), ColIndex, X, Y);
		return FPieceWithStart();
	}

	const auto& Column = Columns[ColIndex];
	int32 CurZ = 0;
	for (const auto& Piece : Column.Pieces)
	{
		if (CurZ + Piece.Size >= Z)
		{
			return FPieceWithStart(Piece, CurZ);
		}

		CurZ += Piece.Size;
	}

	UE_LOG(LogChunk, Warning, TEXT("GetPieceCopy: No piece found at %d,%d,%d"), X, Y, Z);
	return FPieceWithStart();
}

void UChunkData::Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece,
	TArray<uint16>& OutRemovedPiecesZ, TPair<TOptional<FChangeFromSet>, TOptional<FChangeFromSet>>& OutChangedPieces)
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
			Insert.Emplace(Column.Pieces[Idx].MaterialId, AmountBeforeStart);
			
			OutChangedPieces.Key.Emplace(Column.Pieces[Idx].MaterialId, CurZ, Column.Pieces[Idx].Size - AmountBeforeStart, 0);
		}
	}

	// Advance until the end of the new piece
	while (Idx < Column.Pieces.Num() &&
		   CurZ + Column.Pieces[Idx].Size < NewEnd)
	{
		OutRemovedPiecesZ.Add(CurZ);
		CurZ += Column.Pieces[Idx].Size;
		++Idx;
	}

	Insert.Emplace(Piece.MaterialId, Piece.Size);

	const int32 StopReplaceIdx = Idx;
	if (Idx < Column.Pieces.Num())
	{
		const int32 PieceEndZ = CurZ + Column.Pieces[Idx].Size;
		const int32 AmountAfterEnd = FMath::Max<int32>(0, PieceEndZ - static_cast<int32>(NewEnd));
		if (AmountAfterEnd > 0)
		{
			OutChangedPieces.Value.Emplace(Column.Pieces[Idx].MaterialId, CurZ, Column.Pieces[Idx].Size - AmountAfterEnd, AmountAfterEnd);
			Insert.Emplace(Column.Pieces[Idx].MaterialId, AmountAfterEnd);
		} else if (CurZ < NewEnd) {
			// Fully covered terminal piece whose end == NewEnd
			OutRemovedPiecesZ.Add(CurZ);
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
		if (Column.Pieces[i - 1].MaterialId == Column.Pieces[i].MaterialId)
		{
			Column.Pieces[i - 1].Size += Column.Pieces[i].Size;
			Column.Pieces.RemoveAt(i);
			continue;
		}

		++i;
	}

	Changes++;
}

void UChunkData::Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece)
{
	TArray<uint16> Tmp;
	TPair<TOptional<FChangeFromSet>, TOptional<FChangeFromSet>> Tmp2;
	Th_SetPiece(X, Y, Z, Piece, Tmp, Tmp2);
}

void UChunkData::BuildSpatialIndex()
{
	InstanceSpatialIndex.Empty();

	for (auto& [AssetId, Collection] : InstanceCollections)
	{
		for (int32 i = 0; i < Collection.Instances.Num(); i++)
		{
			const FVector LocalPos = Collection.Instances[i].Transform.GetLocation();
			const FIntVector GridPos = GetGridPosition(LocalPos);
			InstanceSpatialIndex.Add(GridPos, TPair<FPrimaryAssetId, int32>(AssetId, i));
		}
	}
}

bool UChunkData::HasEntityAt(const FVector& LocalPosition) const
{
	const FIntVector GridPos = GetGridPosition(LocalPosition);
	const TWeakObjectPtr<AEntityFacade>* EntityPtr = EntityGrid.Find(GridPos);
	return EntityPtr && EntityPtr->IsValid();
}

void UChunkData::RegisterEntity(const FVector& LocalPosition, AEntityFacade* Entity)
{
	if (Entity)
	{
		const FIntVector GridPos = GetGridPosition(LocalPosition);
		EntityGrid.Add(GridPos, Entity);
	}
}

void UChunkData::UnregisterEntity(const FVector& LocalPosition)
{
	const FIntVector GridPos = GetGridPosition(LocalPosition);
	EntityGrid.Remove(GridPos);
}

bool UChunkData::HasWorldItemAt(const FVector& LocalPosition) const
{
	const FIntVector GridPos = GetGridPosition(LocalPosition);
	const TWeakObjectPtr<AItemWorldActor>* ItemPtr = WorldItemGrid.Find(GridPos);
	return ItemPtr && ItemPtr->IsValid();
}

void UChunkData::RegisterWorldItem(const FVector& LocalPosition, AItemWorldActor* WorldItem)
{
	if (WorldItem)
	{
		const FIntVector GridPos = GetGridPosition(LocalPosition);
		WorldItemGrid.Add(GridPos, WorldItem);
	}
}

void UChunkData::UnregisterWorldItem(const FVector& LocalPosition)
{
	const FIntVector GridPos = GetGridPosition(LocalPosition);
	WorldItemGrid.Remove(GridPos);
}

TArray<AItemWorldActor*> UChunkData::GetWorldItems() const
{
	TArray<AItemWorldActor*> Result;
	for (const auto& Pair : WorldItemGrid)
	{
		if (AItemWorldActor* Item = Pair.Value.Get())
		{
			Result.Add(Item);
		}
	}
	return Result;
}
