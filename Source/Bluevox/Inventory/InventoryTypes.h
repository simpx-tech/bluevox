// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EInventoryQuerySort : uint8
{
	Name		UMETA(DisplayName = "Name (A→Z)"),
	MostAmount	UMETA(DisplayName = "Most Amount"),
	MostRecent	UMETA(DisplayName = "Most Recent"),
	MostWeight	UMETA(DisplayName = "Most Weight")
};

USTRUCT(BlueprintType)
struct BLUEVOX_API FInventoryQueryParams
{
	GENERATED_BODY()

	/** Sorting mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Query")
	EInventoryQuerySort SortBy = EInventoryQuerySort::MostRecent;

	/** Optional case-insensitive substring to filter by item display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Query")
	FString NameFilter;
};

class UItemTypeDataAsset;
struct FInventoryItemArray;

/**
 * Represents a single item stack in an inventory
 * Uses FastArraySerializer for efficient network replication
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FInventoryItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** The type of item */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UItemTypeDataAsset> ItemType;

	/** Amount of items in this stack */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Amount = 1;

	/** Cached total weight for performance */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	float TotalWeight = 0.0f;

	/** Last time this stack was modified by an add/stack operation (UTC). Not persisted to save files yet. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FDateTime LastModifiedAt = FDateTime{};

	/** True only for stacks that have just appeared in inventory and haven't been acknowledged yet. Not persisted. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsNew = false;

	FInventoryItem()
		: Amount(1), TotalWeight(0.0f), LastModifiedAt(FDateTime::UtcNow()), bIsNew(false)
	{
	}

	FInventoryItem(const TSoftObjectPtr<UItemTypeDataAsset>& InItemType, int32 InAmount)
		: ItemType(InItemType), Amount(InAmount), TotalWeight(0.0f), LastModifiedAt(FDateTime::UtcNow()), bIsNew(false)
	{
	}

	/** Custom serialization for save/load */
	friend FArchive& operator<<(FArchive& Ar, FInventoryItem& Item)
	{
		// Serialize as string for compatibility
		FString AssetPath;
		if (Ar.IsSaving())
		{
			AssetPath = Item.ItemType.ToString();
		}
		Ar << AssetPath;
		if (Ar.IsLoading())
		{
			Item.ItemType = TSoftObjectPtr<UItemTypeDataAsset>(FSoftObjectPath(AssetPath));
		}
		Ar << Item.Amount;
		Ar << Item.TotalWeight;
		// Note: LastModifiedAt is intentionally NOT serialized to preserve backward compatibility with existing saves.
		return Ar;
	}

	/** Equality operator for finding items */
	bool operator==(const FInventoryItem& Other) const
	{
		return ItemType == Other.ItemType;
	}

	void PostReplicatedAdd(const FInventoryItemArray& InArray);
	void PostReplicatedChange(const FInventoryItemArray& InArray);
	void PreReplicatedRemove(const FInventoryItemArray& InArray);
};

/**
 * Fast array container for inventory items
 * Provides efficient delta-based network replication
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FInventoryItemArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryItem> Items;

	class UInventoryComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryItem, FInventoryItemArray>(Items, DeltaParms, *this);
	}

	void BroadcastChanged() const;
};

template<>
struct TStructOpsTypeTraits<FInventoryItemArray> : public TStructOpsTypeTraitsBase2<FInventoryItemArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
