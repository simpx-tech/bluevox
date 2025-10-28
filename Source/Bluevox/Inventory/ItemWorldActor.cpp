// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemWorldActor.h"
#include "Bluevox/Data/ItemTypeDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AItemWorldActor::AItemWorldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = false;
	SetNetCullDistanceSquared(FMath::Square(5000.0f));

	// Create root component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetSimulatePhysics(true);

	// Create merge detection sphere
	MergeDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MergeDetectionSphere"));
	MergeDetectionSphere->SetupAttachment(MeshComponent);
	MergeDetectionSphere->SetSphereRadius(MergeDetectionRadius);
	MergeDetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MergeDetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	MergeDetectionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	bCanBePickedUp = true;
	StackAmount = 1;
	MergeCheckDelay = 0.5f;
	bIsMerging = false;
}

void AItemWorldActor::BeginPlay()
{
	Super::BeginPlay();

	// Update mesh on start
	UpdateMeshForStackAmount();

	// Start merge checking on server
	if (Sv_IsServer())
	{
		GetWorldTimerManager().SetTimer(
			MergeCheckTimerHandle,
			this,
			&AItemWorldActor::CheckForNearbyItemsToMerge,
			MergeCheckDelay,
			false
		);
	}
}

void AItemWorldActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemWorldActor, ItemType);
	DOREPLIFETIME(AItemWorldActor, StackAmount);
	DOREPLIFETIME(AItemWorldActor, bCanBePickedUp);
}

void AItemWorldActor::InitializeItem(const TSoftObjectPtr<UItemTypeDataAsset>& InItemType, int32 InAmount)
{
	ItemType = InItemType;
	StackAmount = FMath::Max(1, InAmount);

	UpdateMeshForStackAmount();

	// Update merge detection radius
	if (MergeDetectionSphere)
	{
		MergeDetectionSphere->SetSphereRadius(MergeDetectionRadius);
	}
}

void AItemWorldActor::AddToStack(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	StackAmount += Amount;

	// Trigger replication and mesh update
	OnRep_StackAmount();
}

bool AItemWorldActor::RemoveFromStack(int32 Amount)
{
	if (Amount <= 0 || Amount > StackAmount)
	{
		return false;
	}

	StackAmount -= Amount;

	if (StackAmount <= 0)
	{
		// Destroy the actor if no items remain
		Destroy();
		return true;
	}

	// Trigger replication and mesh update
	OnRep_StackAmount();

	return true;
}

void AItemWorldActor::SetCanBePickedUp(bool bCanPickup)
{
	bCanBePickedUp = bCanPickup;
}

void AItemWorldActor::UpdateMeshForStackAmount()
{
	if (!ItemType.IsValid())
	{
		return;
	}

	UItemTypeDataAsset* ItemAsset = ItemType.LoadSynchronous();
	if (!ItemAsset || !MeshComponent)
	{
		return;
	}

	// Get the appropriate mesh for the current stack amount
	UStaticMesh* NewMesh = ItemAsset->GetMeshForStackAmount(StackAmount);
	if (NewMesh)
	{
		MeshComponent->SetStaticMesh(NewMesh);
	}
}

void AItemWorldActor::OnRep_StackAmount()
{
	UpdateMeshForStackAmount();
}

void AItemWorldActor::CheckForNearbyItemsToMerge()
{
	if (!Sv_IsServer() || bIsMerging || !ItemType.IsValid())
	{
		return;
	}

	// Find overlapping actors
	TArray<AActor*> OverlappingActors;
	MergeDetectionSphere->GetOverlappingActors(OverlappingActors, AItemWorldActor::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		AItemWorldActor* OtherItem = Cast<AItemWorldActor>(Actor);
		if (!OtherItem || OtherItem == this || OtherItem->bIsMerging)
		{
			continue;
		}

		// Check if same item type
		if (OtherItem->ItemType == ItemType)
		{
			MergeWithItem(OtherItem);
			break; // Only merge with one item at a time
		}
	}
}

void AItemWorldActor::MergeWithItem(AItemWorldActor* OtherItem)
{
	if (!Sv_IsServer() || !OtherItem || OtherItem == this || bIsMerging || OtherItem->bIsMerging)
	{
		return;
	}

	// Prevent infinite merge loops
	bIsMerging = true;
	OtherItem->bIsMerging = true;

	// Add the other item's stack to this one
	AddToStack(OtherItem->StackAmount);

	// Destroy the other item
	OtherItem->Destroy();

	// Reset merge flag
	bIsMerging = false;

	// Schedule another merge check in case there are more items nearby
	GetWorldTimerManager().SetTimer(
		MergeCheckTimerHandle,
		this,
		&AItemWorldActor::CheckForNearbyItemsToMerge,
		MergeCheckDelay,
		false
	);
}

void AItemWorldActor::RSv_MergeWithItem_Implementation(AItemWorldActor* OtherItem)
{
	MergeWithItem(OtherItem);
}

bool AItemWorldActor::Sv_IsServer() const
{
	return HasAuthority();
}
