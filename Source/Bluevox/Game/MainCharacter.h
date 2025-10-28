// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "MainCharacter.generated.h"

class AGameManager;
class UInputAction;
class UInventoryComponent;
struct FInputActionValue;

UCLASS()
class BLUEVOX_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY()
	AGameManager* GameManager = nullptr;

public:
	// Sets default values for this character's properties
	AMainCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HorizontalSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LookSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bReady = false;

	/** Player's inventory component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* InventoryComponent = nullptr;

	// ========== Quick Inventory ==========

	/**
	 * Quick inventory slots (8 slots) containing indices to items in the main inventory
	 * -1 means the slot is empty (no item assigned)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Quick Inventory")
	TArray<int32> QuickInventorySlots;

	/**
	 * Index of the currently held quick slot (0-7), or -1 if no item is held
	 * Replicated so other players can see what item this player is holding
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Quick Inventory")
	int32 CurrentlyHeldSlot = -1;

	UFUNCTION()
	void HandleMoveAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleLookAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleLeftClickAction();

	UFUNCTION()
	void HandleRightClickAction();
	
	UFUNCTION()
	void HandleJumpAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandleCrouchAction(const FInputActionValue& Value);

	UFUNCTION()
	void HandlePickupItemAction();

	UFUNCTION(Server, Reliable)
	void RSv_PickupItem(AItemWorldActor* ItemActor);

	// Quick slot input handlers (called when player presses 1-8)
	UFUNCTION()
	void HandleQuickSlot1Action();

	UFUNCTION()
	void HandleQuickSlot2Action();

	UFUNCTION()
	void HandleQuickSlot3Action();

	UFUNCTION()
	void HandleQuickSlot4Action();

	UFUNCTION()
	void HandleQuickSlot5Action();

	UFUNCTION()
	void HandleQuickSlot6Action();

	UFUNCTION()
	void HandleQuickSlot7Action();

	UFUNCTION()
	void HandleQuickSlot8Action();

	// ========== Quick Inventory Methods ==========

	/**
	 * Handle quick slot input (1-8)
	 * Toggles between holding and not holding the item in that slot
	 * Client-side call that sends RPC to server
	 */
	UFUNCTION(BlueprintCallable, Category = "Quick Inventory")
	void Cl_HandleQuickSlotInput(int32 SlotIndex);

	/**
	 * Server RPC to select or deselect a quick inventory slot
	 * @param SlotIndex The slot to select (0-7), or -1 to deselect
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Quick Inventory")
	void RSv_SetHeldQuickSlot(int32 SlotIndex);

	/**
	 * Assign an inventory item index to a quick slot
	 * @param QuickSlotIndex The quick slot (0-7) to assign to
	 * @param InventoryItemIndex The index in the main inventory array, or -1 to clear
	 */
	UFUNCTION(BlueprintCallable, Category = "Quick Inventory")
	void Sv_SetQuickSlot(int32 QuickSlotIndex, int32 InventoryItemIndex);

	/**
	 * Get the item type currently held in hand (if any)
	 * @return The item type data asset, or nullptr if no item is held
	 */
	UFUNCTION(BlueprintPure, Category = "Quick Inventory")
	class UItemTypeDataAsset* GetCurrentlyHeldItem() const;

	/**
	 * Get the item type in a specific quick slot
	 * @param SlotIndex The quick slot (0-7) to check
	 * @return The item type data asset, or nullptr if slot is empty
	 */
	UFUNCTION(BlueprintPure, Category = "Quick Inventory")
	UItemTypeDataAsset* GetItemInQuickSlot(int32 SlotIndex) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
