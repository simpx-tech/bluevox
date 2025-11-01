// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UItemTypeDataAsset;
class AItemWorldActor;

// Delegates for inventory changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemAdded, TSoftObjectPtr<UItemTypeDataAsset>, ItemType, int32, Amount, int32, NewTotalAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemRemoved, TSoftObjectPtr<UItemTypeDataAsset>, ItemType, int32, Amount, int32, RemainingAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryFull, TSoftObjectPtr<UItemTypeDataAsset>, ItemType, float, RequiredWeight);

/**
 * General-purpose weight-based inventory component
 * Can be attached to any actor (players, NPCs, containers, etc.)
 * Features unlimited auto-stacking, replication, and save/load support
 */
UCLASS(BlueprintType)
class BLUEVOX_API UInventoryComponent : public UObject
{
	GENERATED_BODY()

	friend struct FInventoryItemArray;

private:
	/** Fast array of items in this inventory */
	UPROPERTY()
	FInventoryItemArray ItemArray;

	/** Maximum weight this inventory can hold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxWeight = 100.0f;

	/** Current total weight of all items */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	float CurrentWeight = 0.0f;

	/** The actor that owns this inventory component */
	UPROPERTY()
	AActor* Owner = nullptr;

public:
	UInventoryComponent();

	/**
	 * Initialize the inventory component
	 * @param InOwner The actor that owns this inventory
	 * @param InMaxWeight Maximum weight capacity
	 * @return This component for chaining
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryComponent* Init(AActor* InOwner, float InMaxWeight);

	// ========== Core Operations ==========

	/**
	 * Add items to the inventory (SERVER ONLY)
	 * Automatically stacks with existing items of the same type
	 * @param ItemType The type of item to add
	 * @param Amount Number of items to add
	 * @return True if items were added successfully, false if not enough space
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Sv_AddItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount = 1);

	/**
	 * Remove items from the inventory (SERVER ONLY)
	 * @param ItemType The type of item to remove
	 * @param Amount Number of items to remove
	 * @return True if items were removed successfully, false if not enough items
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Sv_RemoveItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount = 1);

	/**
	 * Drop items from the inventory into the world (SERVER ONLY)
	 * Spawns an ItemWorldActor at the specified location
	 * @param ItemType The type of item to drop
	 * @param Amount Number of items to drop
	 * @param SpawnLocation World location to spawn the item actor
	 * @return The spawned item actor, or nullptr if failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AItemWorldActor* Sv_DropItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount, const FVector& SpawnLocation);

	/**
	 * Clear all items from the inventory (SERVER ONLY)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Sv_ClearInventory();

	// ========== Helper Functions ==========

	/**
	 * Check if the inventory has a certain amount of an item
	 * @param ItemType The type of item to check
	 * @param Amount The amount to check for
	 * @return True if the inventory has at least that amount
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount = 1) const;

	/**
	 * Get the current count of a specific item type
	 * @param ItemType The type of item to count
	 * @return Total amount of that item in the inventory
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType) const;

	/**
	 * Check if a specific weight can fit in the inventory
	 * @param Weight The weight to check
	 * @return True if the weight can fit
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanFitWeight(float Weight) const;

	/**
	 * Check if a specific amount of an item can fit in the inventory
	 * @param ItemType The type of item to check
	 * @param Amount The amount to check
	 * @return True if the items can fit
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanFitItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount = 1) const;

	/**
	 * Calculate how many items of a specific type can fit in the inventory
	 * @param ItemType The type of item to check
	 * @return Maximum amount that can fit based on weight
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxFitAmount(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType) const;

	/**
	 * Get the remaining weight capacity
	 * @return Available weight space
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetRemainingCapacity() const
	{
		return FMath::Max(0.0f, MaxWeight - CurrentWeight);
	}

	// ========== Getters ==========
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const { return CurrentWeight; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetMaxWeight() const { return MaxWeight; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryItem>& GetItems() const { return ItemArray.Items; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	AActor* GetOwner() const { return Owner; }

	/**
	 * Get the item at the given slot index.
	 * @param Index Slot index within the internal items array (0-based)
	 * @param OutItem Filled with a copy of the item if found
	 * @return true if Index was valid and OutItem was filled; false otherwise (OutItem reset)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItemAtIndex(int32 Index, FInventoryItem& OutItem) const;

	// ========== Query ==========
	/**
	 * Query items using sorting and optional name filter.
	 * Name filter performs case-insensitive substring match on the item DisplayName (falls back to asset path if unloaded).
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	TArray<FInventoryItem> Query(const FInventoryQueryParams& Params) const;

	// ========== Setters ==========

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetMaxWeight(float NewMaxWeight);

	// ========== New Item Acknowledgement ==========
	/** Explicitly clear the new flag for a specific item type (SERVER ONLY). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|New")
	void Sv_MarkItemSeen(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType);
	/** Clear the new flag for all items in this inventory (SERVER ONLY). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|New")
	void Sv_MarkAllItemsSeen();

	// ========== Events ==========

	/** Called when items are added to the inventory */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemAdded OnItemAdded;

	/** Called when items are removed from the inventory */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemRemoved OnItemRemoved;

	/** Called when the inventory changes in any way */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** Called when trying to add items but inventory is full */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryFull OnInventoryFull;

	// ========== Replication ==========

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }

	// ========== Serialization ==========

	virtual void Serialize(FArchive& Ar) override;

private:
	/**
	 * Recalculate the total weight of all items
	 * Called after any inventory change
	 */
	void RecalculateWeight();

	/**
	 * Update the cached weight for a specific item
	 * @param Item The item to update
	 */
	void UpdateItemWeight(FInventoryItem& Item);

	/**
	 * Broadcast that the inventory has changed
	 */
	void BroadcastInventoryChanged();

	/**
	 * Find an existing item stack of the given type
	 * @param ItemType The type to search for
	 * @return Pointer to the item if found, nullptr otherwise
	 */
	FInventoryItem* FindItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType);

	/**
	 * Check if running on server
	 */
	bool Sv_IsServer() const;
};
