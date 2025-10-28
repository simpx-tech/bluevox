// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "InstanceData.h"
#include "PieceWithStart.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Chunk/Position/LocalColumnPosition.h"
#include "Bluevox/Chunk/Position/LocalPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Tick/GameTickable.h"
#include "UObject/Object.h"
#include "ChunkData.generated.h"

struct FGlobalPosition;
class AGameManager;

USTRUCT()
struct FChangeFromSet
{
	GENERATED_BODY()

	UPROPERTY()
	EMaterial MaterialId;
	
	UPROPERTY()
	uint16 PositionZ;

	UPROPERTY()
	int32 SizeChange;

	UPROPERTY()
	int32 StartChange;
};

/**
 *
 */
UCLASS()
class BLUEVOX_API UChunkData : public UObject
{
	GENERATED_BODY()
	
public:
	UChunkData* Init(AGameManager* InGameManager, const FChunkPosition InPosition, TArray<FChunkColumn>&& InColumns,
	                 TMap<FPrimaryAssetId, FInstanceCollection>&& InInstances = TMap<FPrimaryAssetId, FInstanceCollection>());

	UPROPERTY()
	TArray<FChunkColumn> Columns;

	UPROPERTY()
	TMap<FPrimaryAssetId, FInstanceCollection> InstanceCollections;

	// Entity tracking - maps grid position to entity actor
	UPROPERTY()
	TMap<FIntVector, TWeakObjectPtr<class AEntityFacade>> EntityGrid;

	// Spatial index for fast instance lookups - maps grid pos to (AssetId, InstanceIndex)
	TMap<FIntVector, TPair<FPrimaryAssetId, int32>> InstanceSpatialIndex;

	UPROPERTY()
	AGameManager* GameManager;

	UPROPERTY()
	FChunkPosition Position;

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

	int32 GetFirstGapThatFits(const FGlobalPosition& GlobalPosition, const int32 FitHeightInLayers);
	
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

	inline FPieceWithStart Th_GetPieceCopy(FLocalPosition LocalPosition);
	
	FPieceWithStart Th_GetPieceCopy(const int32 X, const int32 Y, const int32 Z);

	void Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece, TArray<uint16>& OutRemovedPiecesZ, TPair<TOptional<FChangeFromSet>, TOptional<FChangeFromSet>>& OutChangedPieces);

	void Th_SetPiece(const int32 X, const int32 Y, const int32 Z, const FPiece& Piece);

	// Spatial indexing for instances and entities
	void BuildSpatialIndex();

	// Check if entity exists at grid position
	bool HasEntityAt(const FVector& LocalPosition) const;

	// Register an entity at a position
	void RegisterEntity(const FVector& LocalPosition, class AEntityFacade* Entity);

	// Remove entity from grid
	void UnregisterEntity(const FVector& LocalPosition);

	// Get grid position from local coordinates
	static FIntVector GetGridPosition(const FVector& LocalPosition)
	{
		return FIntVector(LocalPosition / 100.0f);
	}
};
