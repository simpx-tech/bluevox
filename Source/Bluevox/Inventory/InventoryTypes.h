// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryTypes.generated.h"

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

	FInventoryItem()
		: Amount(1), TotalWeight(0.0f)
	{
	}

	FInventoryItem(const TSoftObjectPtr<UItemTypeDataAsset>& InItemType, int32 InAmount)
		: ItemType(InItemType), Amount(InAmount), TotalWeight(0.0f)
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
