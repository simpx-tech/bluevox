// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkColumn.h"
#include "PieceWithStart.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "Bluevox/Chunk/Position/LocalColumnPosition.h"
#include "Bluevox/Chunk/Position/LocalPosition.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Entity/EntityTypes.h"
#include "UObject/Object.h"
#include "ChunkData.generated.h"

struct FGlobalPosition;
class AGameManager;
class AItemWorldActor;
class UItemTypeDataAsset;

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
 * Serializable struct for storing world item data
 */
USTRUCT()
struct FWorldItemData
{
	GENERATED_BODY()

	/** Item type asset reference */
	UPROPERTY()
	TSoftObjectPtr<UItemTypeDataAsset> ItemType;

	/** Stack amount */
	UPROPERTY()
	int32 StackAmount = 1;

	/** World location */
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** World rotation */
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	FWorldItemData() = default;

	FWorldItemData(const TSoftObjectPtr<UItemTypeDataAsset>& InItemType, int32 InStackAmount, const FVector& InLocation, const FRotator& InRotation)
		: ItemType(InItemType), StackAmount(InStackAmount), Location(InLocation), Rotation(InRotation)
	{
	}

	/** Custom serialization */
	friend FArchive& operator<<(FArchive& Ar, FWorldItemData& ItemData)
	{
		// Serialize as string for compatibility
		FString AssetPath;
		if (Ar.IsSaving())
		{
			AssetPath = ItemData.ItemType.ToString();
		}
		Ar << AssetPath;
		if (Ar.IsLoading())
		{
			ItemData.ItemType = TSoftObjectPtr<UItemTypeDataAsset>(FSoftObjectPath(AssetPath));
		}
		Ar << ItemData.StackAmount;
		Ar << ItemData.Location;
		Ar << ItemData.Rotation;
		return Ar;
	}
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
	                 TArray<FEntityRecord>&& InEntities = TArray<FEntityRecord>());

	UPROPERTY()
	TArray<FChunkColumn> Columns;

	// Persistent entity records stored in this chunk (server authoritative)
	TSparseArray<FEntityRecord> Entities;

	// World item tracking - maps grid position to item actor
	UPROPERTY()
	TMap<FIntVector, TWeakObjectPtr<AItemWorldActor>> WorldItemGrid;

	UPROPERTY()
	AGameManager* GameManager;

	UPROPERTY()
	FChunkPosition Position;

	UPROPERTY()
	int32 Changes = 0;

	FRWLock Lock;
	
	virtual void Serialize(FArchive& Ar) override;

	void SerializeForSave(FArchive& Ar);

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

	// Check if world item exists at grid position
	bool HasWorldItemAt(const FVector& LocalPosition) const;

	// Register a world item at a position
	void RegisterWorldItem(const FVector& LocalPosition, AItemWorldActor* WorldItem);

	// Remove world item from grid
	void UnregisterWorldItem(const FVector& LocalPosition);

	// Get all world items in this chunk
	TArray<AItemWorldActor*> GetWorldItems() const;

	// Get grid position from local coordinates
	static FIntVector GetGridPosition(const FVector& LocalPosition)
	{
		return FIntVector(LocalPosition / 100.0f);
	}
};
