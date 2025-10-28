// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"
#include "ItemWorldActor.h"
#include "Bluevox/Data/ItemTypeDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

// ========== FInventoryItem Replication Callbacks ==========

void FInventoryItem::PostReplicatedAdd(const FInventoryItemArray& InArray)
{
	InArray.BroadcastChanged();
}

void FInventoryItem::PostReplicatedChange(const FInventoryItemArray& InArray)
{
	InArray.BroadcastChanged();
}

void FInventoryItem::PreReplicatedRemove(const FInventoryItemArray& InArray)
{
	InArray.BroadcastChanged();
}

// ========== FInventoryItemArray Implementation ==========

void FInventoryItemArray::BroadcastChanged() const
{
	if (Owner)
	{
		Owner->BroadcastInventoryChanged();
	}
}

// ========== UInventoryComponent Implementation ==========

UInventoryComponent::UInventoryComponent()
{
	MaxWeight = 100.0f;
	CurrentWeight = 0.0f;
}

UInventoryComponent* UInventoryComponent::Init(AActor* InOwner, float InMaxWeight)
{
	Owner = InOwner;
	MaxWeight = InMaxWeight;
	CurrentWeight = 0.0f;
	ItemArray.Items.Empty();
	ItemArray.Owner = this;

	return this;
}

bool UInventoryComponent::Sv_AddItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount)
{
	if (!Sv_IsServer())
	{
		return false;
	}

	if (!ItemType.IsValid() || Amount <= 0)
	{
		return false;
	}

	// Load the asset if not already loaded
	UItemTypeDataAsset* ItemAsset = ItemType.LoadSynchronous();
	if (!ItemAsset)
	{
		return false;
	}

	const float ItemWeight = ItemAsset->GetWeightForAmount(Amount);

	// Check if we have enough space
	if (!CanFitWeight(ItemWeight))
	{
		OnInventoryFull.Broadcast(ItemType, ItemWeight);
		return false;
	}

	// Try to find existing stack
	FInventoryItem* ExistingItem = FindItem(ItemType);
	if (ExistingItem)
	{
		// Stack with existing item
		ExistingItem->Amount += Amount;
		UpdateItemWeight(*ExistingItem);
		ItemArray.MarkItemDirty(*ExistingItem);
	}
	else
	{
		// Create new stack
		FInventoryItem NewItem(ItemType, Amount);
		UpdateItemWeight(NewItem);
		ItemArray.Items.Add(NewItem);
		ItemArray.MarkArrayDirty();
	}

	RecalculateWeight();

	const int32 NewTotalAmount = GetItemCount(ItemType);
	OnItemAdded.Broadcast(ItemType, Amount, NewTotalAmount);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::Sv_RemoveItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount)
{
	if (!Sv_IsServer())
	{
		return false;
	}

	if (!ItemType.IsValid() || Amount <= 0)
	{
		return false;
	}

	FInventoryItem* ExistingItem = FindItem(ItemType);
	if (!ExistingItem || ExistingItem->Amount < Amount)
	{
		return false;
	}

	ExistingItem->Amount -= Amount;
	int32 RemainingAmount = ExistingItem->Amount;

	if (ExistingItem->Amount <= 0)
	{
		// Remove the item completely
		ItemArray.Items.RemoveAll([&ItemType](const FInventoryItem& Item)
		{
			return Item.ItemType == ItemType;
		});
		ItemArray.MarkArrayDirty();
		RemainingAmount = 0;
	}
	else
	{
		UpdateItemWeight(*ExistingItem);
		ItemArray.MarkItemDirty(*ExistingItem);
	}

	RecalculateWeight();

	OnItemRemoved.Broadcast(ItemType, Amount, RemainingAmount);
	BroadcastInventoryChanged();

	return true;
}

AItemWorldActor* UInventoryComponent::Sv_DropItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount, const FVector& SpawnLocation)
{
	if (!Sv_IsServer())
	{
		return nullptr;
	}

	if (!Sv_RemoveItem(ItemType, Amount))
	{
		return nullptr;
	}

	// Spawn the item actor in the world
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemWorldActor* SpawnedItem = World->SpawnActor<AItemWorldActor>(AItemWorldActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedItem)
	{
		SpawnedItem->InitializeItem(ItemType, Amount);
	}

	return SpawnedItem;
}

void UInventoryComponent::Sv_ClearInventory()
{
	if (!Sv_IsServer())
	{
		return;
	}

	ItemArray.Items.Empty();
	ItemArray.MarkArrayDirty();
	CurrentWeight = 0.0f;
	BroadcastInventoryChanged();
}

bool UInventoryComponent::HasItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount) const
{
	return GetItemCount(ItemType) >= Amount;
}

int32 UInventoryComponent::GetItemCount(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType) const
{
	for (const FInventoryItem& Item : ItemArray.Items)
	{
		if (Item.ItemType == ItemType)
		{
			return Item.Amount;
		}
	}
	return 0;
}

bool UInventoryComponent::CanFitWeight(float Weight) const
{
	return (CurrentWeight + Weight) <= MaxWeight;
}

bool UInventoryComponent::CanFitItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType, int32 Amount) const
{
	if (!ItemType.IsValid())
	{
		return false;
	}

	UItemTypeDataAsset* ItemAsset = ItemType.LoadSynchronous();
	if (!ItemAsset)
	{
		return false;
	}

	const float ItemWeight = ItemAsset->GetWeightForAmount(Amount);
	return CanFitWeight(ItemWeight);
}

int32 UInventoryComponent::GetMaxFitAmount(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType) const
{
	if (!ItemType.IsValid())
	{
		return 0;
	}

	UItemTypeDataAsset* ItemAsset = ItemType.LoadSynchronous();
	if (!ItemAsset || ItemAsset->Weight <= 0.0f)
	{
		return 0;
	}

	const float RemainingWeight = GetRemainingCapacity();
	return FMath::FloorToInt(RemainingWeight / ItemAsset->Weight);
}

void UInventoryComponent::SetMaxWeight(float NewMaxWeight)
{
	MaxWeight = FMath::Max(0.0f, NewMaxWeight);

	// Recalculate in case we're now over capacity
	RecalculateWeight();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, ItemArray);
	DOREPLIFETIME(UInventoryComponent, MaxWeight);
	DOREPLIFETIME(UInventoryComponent, CurrentWeight);
}

void UInventoryComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	int32 FileVersion = 1; // Inventory system version
	Ar << FileVersion;

	if (FileVersion >= 1)
	{
		Ar << ItemArray.Items;
		Ar << MaxWeight;
		Ar << CurrentWeight;
	}
}

void UInventoryComponent::RecalculateWeight()
{
	CurrentWeight = 0.0f;

	for (FInventoryItem& Item : ItemArray.Items)
	{
		UpdateItemWeight(Item);
		CurrentWeight += Item.TotalWeight;
	}
}

void UInventoryComponent::UpdateItemWeight(FInventoryItem& Item)
{
	if (!Item.ItemType.IsValid())
	{
		Item.TotalWeight = 0.0f;
		return;
	}

	UItemTypeDataAsset* ItemAsset = Item.ItemType.LoadSynchronous();
	if (ItemAsset)
	{
		Item.TotalWeight = ItemAsset->GetWeightForAmount(Item.Amount);
	}
	else
	{
		Item.TotalWeight = 0.0f;
	}
}

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

FInventoryItem* UInventoryComponent::FindItem(const TSoftObjectPtr<UItemTypeDataAsset>& ItemType)
{
	for (FInventoryItem& Item : ItemArray.Items)
	{
		if (Item.ItemType == ItemType)
		{
			return &Item;
		}
	}
	return nullptr;
}

bool UInventoryComponent::Sv_IsServer() const
{
	UWorld* World = GetWorld();
	return World && (World->GetNetMode() < NM_Client);
}
