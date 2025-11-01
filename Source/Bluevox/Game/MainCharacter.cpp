// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "MainController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameManager.h"
#include "GameConstants.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Bluevox/Inventory/InventoryComponent.h"
#include "Bluevox/Inventory/ItemWorldActor.h"
#include "Bluevox/Data/ItemTypeDataAsset.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"


// Sets default values
AMainCharacter::AMainCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	const auto MovementComponent = GetCharacterMovement();
	MovementComponent->GravityScale = 0.0f;
	MovementComponent->AirControl = 1.0f;
	MovementComponent->MaxFlySpeed = 1200.f;
	MovementComponent->BrakingDecelerationFlying = 4096.0f;
	MovementComponent->MaxAcceleration = 10'000.f;
	MovementComponent->DefaultLandMovementMode = MOVE_None;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize previous-state caches for replication notifications
	PrevQuickInventorySlots = QuickInventorySlots;
	PrevHeldSlot = CurrentlyHeldSlot;
	
	if (const AMainController* PlayerController = Cast<AMainController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PlayerController->DefaultInputContext, 0);
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::BeginPlay: Controller is not a MainController!"));
	}

	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));

	// Initialize inventory component
	if (!InventoryComponent)
	{
		InventoryComponent = NewObject<UInventoryComponent>(this, TEXT("InventoryComponent"));
		InventoryComponent->Init(this, 100.0f); // Default max weight of 100
	}

	// Initialize quick inventory slots (12 slots, all empty)
	if (QuickInventorySlots.Num() == 0)
	{
		QuickInventorySlots.Init(-1, QuickSlotCount);
	}

	// After initialization, sync previous-state caches
	PrevQuickInventorySlots = QuickInventorySlots;
	PrevHeldSlot = CurrentlyHeldSlot;
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	const auto MainController = Cast<AMainController>(GetController());
	if (MainController)
	{
		if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
		{
			EnhancedInputComponent->BindAction(MainController->MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleMoveAction);

			EnhancedInputComponent->BindAction(MainController->LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleLookAction);

			EnhancedInputComponent->BindAction(MainController->JumpAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleJumpAction);

			EnhancedInputComponent->BindAction(MainController->CrouchAction, ETriggerEvent::Triggered, this, &AMainCharacter::HandleCrouchAction);

			EnhancedInputComponent->BindAction(MainController->LeftClickAction, ETriggerEvent::Started, this, &AMainCharacter::HandleLeftClickAction);

			EnhancedInputComponent->BindAction(MainController->RightClickAction, ETriggerEvent::Started, this, &AMainCharacter::HandleRightClickAction);

			EnhancedInputComponent->BindAction(MainController->PickupItemAction, ETriggerEvent::Started, this, &AMainCharacter::HandlePickupItemAction);

			// Bind quick slot actions (1-4 only)
			EnhancedInputComponent->BindAction(MainController->QuickSlot1Action, ETriggerEvent::Started, this, &AMainCharacter::HandleQuickSlot1Action);
			EnhancedInputComponent->BindAction(MainController->QuickSlot2Action, ETriggerEvent::Started, this, &AMainCharacter::HandleQuickSlot2Action);
			EnhancedInputComponent->BindAction(MainController->QuickSlot3Action, ETriggerEvent::Started, this, &AMainCharacter::HandleQuickSlot3Action);
			EnhancedInputComponent->BindAction(MainController->QuickSlot4Action, ETriggerEvent::Started, this, &AMainCharacter::HandleQuickSlot4Action);
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::SetupPlayerInputComponent: Controller is not a MainController!"));
	}
}

void AMainCharacter::HandleMoveAction(const FInputActionValue& Value)
{
	// const auto Direction = Value.Get<FVector2D>();
	// const auto Forward = GetActorForwardVector();
	// const auto Right = GetActorRightVector();
	//
	// const FVector ForwardMovement = Forward * Direction.Y;
	// const FVector RightMovement = Right * Direction.X;
	//
	// if (Controller)
	// {
	// 	AddMovementInput(ForwardMovement, 1.0f);
	// 	AddMovementInput(RightMovement, 1.0f);
	// }
	FVector2D D = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), D.Y);
	AddMovementInput(GetActorRightVector(),   D.X);
}

void AMainCharacter::HandleLookAction(const FInputActionValue& Value)
{
	const auto LookDelta = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookDelta.X * LookSpeed);
		AddControllerPitchInput(-LookDelta.Y * LookSpeed);
	}
}

void AMainCharacter::HandleLeftClickAction()
{
	GameManager->LocalController->RSv_LeftClick();
}

void AMainCharacter::HandleRightClickAction()
{
	GameManager->LocalController->RSv_RightClick();
}

void AMainCharacter::HandleJumpAction(const FInputActionValue& Value)
{
	// AddMovementInput(FVector::UpVector, VerticalSpeed);
	AddMovementInput(FVector::UpVector, 1.0f);
}

void AMainCharacter::HandleCrouchAction(const FInputActionValue& Value)
{
	// AddMovementInput(FVector::DownVector, VerticalSpeed);
	AddMovementInput(FVector::DownVector, 1.0f);
}

void AMainCharacter::HandlePickupItemAction()
{
	if (!InventoryComponent)
	{
		return;
	}

	// Get player view point
	FVector ViewLoc;
	FRotator ViewRot;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	}
	else
	{
		return;
	}

	// Use GameConstants::Distances::InteractionDistance
	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * GameConstants::Distances::InteractionDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
	Params.AddIgnoredActor(this);

	// Try line trace first for precise targeting
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldDynamic, Params))
	{
		if (AItemWorldActor* ItemActor = Cast<AItemWorldActor>(Hit.GetActor()))
		{
			// Send pickup request to server
			RSv_PickupItem(ItemActor);
			return;
		}
	}

	// If line trace didn't hit an item, try sphere overlap for nearby items
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(100.0f); // 100cm radius
	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, Sphere, Params))
	{
		// Find closest item
		AItemWorldActor* ClosestItem = nullptr;
		float ClosestDistance = GameConstants::Distances::InteractionDistance;

		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AItemWorldActor* ItemActor = Cast<AItemWorldActor>(Overlap.GetActor()))
			{
				const float Distance = FVector::Dist(GetActorLocation(), ItemActor->GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestItem = ItemActor;
					ClosestDistance = Distance;
				}
			}
		}

		// Try to pick up the closest item
		if (ClosestItem)
		{
			RSv_PickupItem(ClosestItem);
		}
	}
}

void AMainCharacter::RSv_PickupItem_Implementation(AItemWorldActor* ItemActor)
{
	if (!ItemActor || !InventoryComponent)
	{
		return;
	}

	// Check if item can be picked up
	if (ItemActor->GetStackAmount() > 0 && InventoryComponent->CanFitItem(ItemActor->GetItemType(), ItemActor->GetStackAmount()))
	{
		// Add to inventory (server-side)
		if (InventoryComponent->Sv_AddItem(ItemActor->GetItemType(), ItemActor->GetStackAmount()))
		{
			// Destroy the world item
			ItemActor->Destroy();
		}
	}
}

// ========== Quick Slot Input Handlers ==========

void AMainCharacter::HandleQuickSlot1Action()
{
	Cl_HandleQuickSlotInput(0);
}

void AMainCharacter::HandleQuickSlot2Action()
{
	Cl_HandleQuickSlotInput(1);
}

void AMainCharacter::HandleQuickSlot3Action()
{
	Cl_HandleQuickSlotInput(2);
}

void AMainCharacter::HandleQuickSlot4Action()
{
	Cl_HandleQuickSlotInput(3);
}

// ========== Quick Inventory Implementation ==========

void AMainCharacter::Cl_HandleQuickSlotInput(int32 SlotIndex)
{
	// Only quick actions 1-4 are supported via input (slots 0-3)
	if (SlotIndex < 0 || SlotIndex > 3)
	{
		UE_LOG(LogTemp, Verbose, TEXT("AMainCharacter::Cl_HandleQuickSlotInput: Ignoring quick slot %d (only 1-4 are bound)"), SlotIndex);
		return;
	}

	// Validate slot index against inventory slots as well
	if (SlotIndex >= QuickSlotCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::Cl_HandleQuickSlotInput: Invalid slot index %d (QuickSlotCount=%d)"), SlotIndex, QuickSlotCount);
		return;
	}

	// Check if this slot has an item assigned
	if (!QuickInventorySlots.IsValidIndex(SlotIndex) || QuickInventorySlots[SlotIndex] < 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("AMainCharacter::Cl_HandleQuickSlotInput: Slot %d is empty"), SlotIndex);
		return;
	}

	// Toggle logic: if pressing the same slot that's currently held, unhold it
	if (CurrentlyHeldSlot == SlotIndex)
	{
		// Unhold the item
		RSv_SetHeldQuickSlot(-1);
	}
	else
	{
		// Hold the item in the selected slot
		RSv_SetHeldQuickSlot(SlotIndex);
	}
}

void AMainCharacter::RSv_SetHeldQuickSlot_Implementation(int32 SlotIndex)
{
	// Server validation
	if (SlotIndex != -1 && (SlotIndex < 0 || SlotIndex >= QuickSlotCount))
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::RSv_SetHeldQuickSlot: Invalid slot index %d"), SlotIndex);
		return;
	}

	// If not unholding (-1), validate the slot has an item
	if (SlotIndex != -1 && (!QuickInventorySlots.IsValidIndex(SlotIndex) || QuickInventorySlots[SlotIndex] < 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::RSv_SetHeldQuickSlot: Slot %d is empty"), SlotIndex);
		return;
	}

	// Update the currently held slot (replicated to all clients)
	const int32 OldSlot = CurrentlyHeldSlot;
	if (OldSlot == SlotIndex)
	{
		return; // no change
	}
	CurrentlyHeldSlot = SlotIndex;

	// Broadcast immediately on server for server-side listeners
	OnHeldQuickSlotChanged.Broadcast(CurrentlyHeldSlot, OldSlot);

	// Keep server-side cache in sync so future diffs are correct
	PrevHeldSlot = CurrentlyHeldSlot;
}

void AMainCharacter::Sv_SetQuickSlot(int32 QuickSlotIndex, int32 InventoryItemIndex)
{
	// Server-side only
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::Sv_SetQuickSlot: Can only be called on server"));
		return;
	}

	// Validate quick slot index
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QuickSlotCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::Sv_SetQuickSlot: Invalid quick slot index %d"), QuickSlotIndex);
		return;
	}

	// Validate inventory item index (can be -1 to clear the slot)
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::Sv_SetQuickSlot: No inventory component"));
		return;
	}

	if (InventoryItemIndex != -1 && (InventoryItemIndex < 0 || InventoryItemIndex >= InventoryComponent->GetItems().Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("AMainCharacter::Sv_SetQuickSlot: Invalid inventory item index %d"), InventoryItemIndex);
		return;
	}

	// Ensure prev cache is sized
	if (PrevQuickInventorySlots.Num() != QuickInventorySlots.Num())
	{
		PrevQuickInventorySlots = QuickInventorySlots;
	}

	// Change the quick slot mapping
	const int32 OldInventoryIndex = QuickInventorySlots.IsValidIndex(QuickSlotIndex) ? QuickInventorySlots[QuickSlotIndex] : -1;
	if (OldInventoryIndex == InventoryItemIndex)
	{
		return; // no change
	}
	QuickInventorySlots[QuickSlotIndex] = InventoryItemIndex;

	// If the currently held slot is being cleared, unhold it
	if (CurrentlyHeldSlot == QuickSlotIndex && InventoryItemIndex == -1)
	{
		const int32 OldHeld = CurrentlyHeldSlot;
		CurrentlyHeldSlot = -1;
		OnHeldQuickSlotChanged.Broadcast(CurrentlyHeldSlot, OldHeld);
		PrevHeldSlot = CurrentlyHeldSlot;
	}

	// Broadcast slot mapping change immediately on server
	OnQuickInventorySlotChanged.Broadcast(QuickSlotIndex, InventoryItemIndex, OldInventoryIndex);
	// Keep prev cache in sync
	if (PrevQuickInventorySlots.IsValidIndex(QuickSlotIndex))
	{
		PrevQuickInventorySlots[QuickSlotIndex] = InventoryItemIndex;
	}
}

UItemTypeDataAsset* AMainCharacter::GetCurrentlyHeldItem() const
{
	if (CurrentlyHeldSlot < 0 || CurrentlyHeldSlot >= QuickSlotCount)
	{
		return nullptr;
	}

	return GetItemInQuickSlot(CurrentlyHeldSlot);
}

UItemTypeDataAsset* AMainCharacter::GetItemInQuickSlot(int32 SlotIndex) const
{
	// Validate slot index
	if (SlotIndex < 0 || SlotIndex >= QuickSlotCount)
	{
		return nullptr;
	}

	// Check if slot has an item assigned
	if (!QuickInventorySlots.IsValidIndex(SlotIndex) || QuickInventorySlots[SlotIndex] < 0)
	{
		return nullptr;
	}

	// Get the inventory item index
	const int32 InventoryItemIndex = QuickInventorySlots[SlotIndex];

	// Validate inventory component and item index
	if (!InventoryComponent || !InventoryComponent->GetItems().IsValidIndex(InventoryItemIndex))
	{
		return nullptr;
	}

	// Get the item type from the inventory
	const FInventoryItem& InventoryItem = InventoryComponent->GetItems()[InventoryItemIndex];
	return InventoryItem.ItemType.LoadSynchronous();
}

void AMainCharacter::OnRep_QuickInventorySlots()
{
	// Ensure previous cache size
	if (PrevQuickInventorySlots.Num() != QuickInventorySlots.Num())
	{
		PrevQuickInventorySlots.SetNum(QuickInventorySlots.Num());
	}

	for (int32 i = 0; i < QuickInventorySlots.Num(); ++i)
	{
		const int32 OldIdx = PrevQuickInventorySlots[i];
		const int32 NewIdx = QuickInventorySlots[i];
		if (OldIdx != NewIdx)
		{
			OnQuickInventorySlotChanged.Broadcast(i, NewIdx, OldIdx);
			PrevQuickInventorySlots[i] = NewIdx;
		}
	}
}

void AMainCharacter::OnRep_CurrentlyHeldSlot()
{
	const int32 Old = PrevHeldSlot;
	OnHeldQuickSlotChanged.Broadcast(CurrentlyHeldSlot, Old);
	PrevHeldSlot = CurrentlyHeldSlot;
}

void AMainCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMainCharacter, bReady);
	DOREPLIFETIME(AMainCharacter, QuickInventorySlots);
	DOREPLIFETIME(AMainCharacter, CurrentlyHeldSlot);
}

