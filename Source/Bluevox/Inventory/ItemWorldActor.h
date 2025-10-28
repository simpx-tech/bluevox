// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemWorldActor.generated.h"

class UItemTypeDataAsset;
class UStaticMeshComponent;
class USphereComponent;

/**
 * Actor representing a dropped item in the world
 * Features:
 * - Replication for multiplayer
 * - Auto-merge with nearby items of the same type
 * - Dynamic mesh updates based on stack amount
 * - No auto-despawn (persists until picked up)
 */
UCLASS()
class BLUEVOX_API AItemWorldActor : public AActor
{
	GENERATED_BODY()

protected:
	/** Visual representation of the item */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	/** Collision sphere for detecting nearby items to merge */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* MergeDetectionSphere;

	/** The type of item this actor represents */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Item")
	TSoftObjectPtr<UItemTypeDataAsset> ItemType;

	/** Number of items in this stack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_StackAmount, Category = "Item", meta = (ClampMin = "1"))
	int32 StackAmount = 1;

	/** Radius for detecting nearby items to merge with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Merge", meta = (ClampMin = "0.0"))
	float MergeDetectionRadius = 150.0f;

	/** Time to wait before checking for nearby items to merge */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Merge", meta = (ClampMin = "0.0"))
	float MergeCheckDelay = 0.5f;

	/** Whether this item can be picked up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	bool bCanBePickedUp = true;

	/** Timer handle for merge checking */
	FTimerHandle MergeCheckTimerHandle;

public:
	AItemWorldActor();

	/**
	 * Initialize the item with a type and amount
	 * @param InItemType The type of item
	 * @param InAmount The stack amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeItem(const TSoftObjectPtr<UItemTypeDataAsset>& InItemType, int32 InAmount);

	/**
	 * Get the item type
	 */
	UFUNCTION(BlueprintPure, Category = "Item")
	TSoftObjectPtr<UItemTypeDataAsset> GetItemType() const { return ItemType; }

	/**
	 * Get the stack amount
	 */
	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetStackAmount() const { return StackAmount; }

	/**
	 * Add more items to this stack
	 * @param Amount Number of items to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void AddToStack(int32 Amount);

	/**
	 * Remove items from this stack
	 * @param Amount Number of items to remove
	 * @return True if successful, false if not enough items
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool RemoveFromStack(int32 Amount);

	/**
	 * Set whether this item can be picked up
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetCanBePickedUp(bool bCanPickup);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Update the visual mesh based on current stack amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void UpdateMeshForStackAmount();

	/**
	 * Called when StackAmount is replicated
	 */
	UFUNCTION()
	void OnRep_StackAmount();

	/**
	 * Check for nearby items of the same type and merge with them
	 */
	UFUNCTION()
	void CheckForNearbyItemsToMerge();

	/**
	 * Merge this item with another item actor
	 * @param OtherItem The item to merge with
	 */
	UFUNCTION()
	void MergeWithItem(AItemWorldActor* OtherItem);

	/**
	 * Server RPC to handle merging
	 */
	UFUNCTION(Server, Reliable)
	void RSv_MergeWithItem(AItemWorldActor* OtherItem);

	/**
	 * Check if running on server
	 */
	bool Sv_IsServer() const;

private:
	/** Flag to prevent infinite merge loops */
	bool bIsMerging = false;
};
