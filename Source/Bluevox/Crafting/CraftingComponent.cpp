// Copyright Epic Games, Inc. All Rights Reserved.

#include "CraftingComponent.h"
#include "WorkingStationInterface.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCraftingComponent, KnownRecipes);
	DOREPLIFETIME(UCraftingComponent, ActiveCraftingOperation);
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Server handles crafting completion
	if (GetOwnerRole() == ROLE_Authority)
	{
		Sv_TickCrafting(DeltaTime);
	}
}

// ===== Recipe Management =====

bool UCraftingComponent::Sv_LearnRecipe(UCraftingRecipeDataAsset* Recipe, bool bInitialLoad)
{
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_LearnRecipe - Recipe is null"));
		return false;
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_LearnRecipe - Called on client, must be server"));
		return false;
	}

	// Check if already known
	if (KnownRecipes.Contains(Recipe))
	{
		return false;
	}

	// Add to known recipes
	KnownRecipes.Add(Recipe);

	// Fire event
	if (!bInitialLoad)
	{
		OnRecipeLearned.Broadcast(Recipe, bInitialLoad);
	}

	UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::Sv_LearnRecipe - Learned recipe: %s"), *Recipe->RecipeName.ToString());
	return true;
}

bool UCraftingComponent::Sv_ForgetRecipe(UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_ForgetRecipe - Recipe is null"));
		return false;
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_ForgetRecipe - Called on client, must be server"));
		return false;
	}

	const int32 RemovedCount = KnownRecipes.Remove(Recipe);

	if (RemovedCount > 0)
	{
		OnRecipeForgotten.Broadcast(Recipe);
		UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::Sv_ForgetRecipe - Forgot recipe: %s"), *Recipe->RecipeName.ToString());
		return true;
	}

	return false;
}

bool UCraftingComponent::HasRecipe(const UCraftingRecipeDataAsset* Recipe) const
{
	return Recipe && KnownRecipes.Contains(Recipe);
}

FCraftingRecipeQueryResult UCraftingComponent::GetKnownRecipes(int32 PageNumber, int32 PageSize,
	FWorkingStationType WorkingStationFilter, FName CategoryFilter) const
{
	FCraftingRecipeQueryResult Result;
	Result.PageNumber = FMath::Max(0, PageNumber);
	Result.PageSize = FMath::Max(1, PageSize);

	// Filter recipes
	TArray<UCraftingRecipeDataAsset*> FilteredRecipes;
	for (UCraftingRecipeDataAsset* Recipe : KnownRecipes)
	{
		if (!Recipe) continue;

		// Apply working station filter
		if (WorkingStationFilter.IsValid())
		{
			if (Recipe->RequiredWorkingStation.StationType != WorkingStationFilter.StationType)
			{
				continue;
			}
		}

		// Apply category filter
		if (CategoryFilter != NAME_None)
		{
			if (Recipe->Category != CategoryFilter)
			{
				continue;
			}
		}

		FilteredRecipes.Add(Recipe);
	}

	// Sort by priority (higher first), then by name
	FilteredRecipes.Sort([](const UCraftingRecipeDataAsset& A, const UCraftingRecipeDataAsset& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.RecipeName.CompareTo(B.RecipeName) < 0;
	});

	Result.TotalCount = FilteredRecipes.Num();

	// Calculate pagination
	const int32 StartIndex = Result.PageNumber * Result.PageSize;
	const int32 EndIndex = FMath::Min(StartIndex + Result.PageSize, FilteredRecipes.Num());

	if (StartIndex < FilteredRecipes.Num())
	{
		for (int32 i = StartIndex; i < EndIndex; ++i)
		{
			Result.Recipes.Add(FilteredRecipes[i]);
		}
	}

	Result.bHasMorePages = EndIndex < FilteredRecipes.Num();

	return Result;
}

int32 UCraftingComponent::GetKnownRecipeCount(FWorkingStationType WorkingStationFilter, FName CategoryFilter) const
{
	if (!WorkingStationFilter.IsValid() && CategoryFilter == NAME_None)
	{
		return KnownRecipes.Num();
	}

	// Count with filters
	int32 Count = 0;
	for (const UCraftingRecipeDataAsset* Recipe : KnownRecipes)
	{
		if (!Recipe) continue;

		if (WorkingStationFilter.IsValid() && Recipe->RequiredWorkingStation.StationType != WorkingStationFilter.StationType)
		{
			continue;
		}

		if (CategoryFilter != NAME_None && Recipe->Category != CategoryFilter)
		{
			continue;
		}

		Count++;
	}

	return Count;
}

// ===== Crafting Operations =====

bool UCraftingComponent::StartCrafting(UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation)
{
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::StartCrafting - Recipe is null"));
		return false;
	}

	// Call server RPC
	if (GetOwnerRole() != ROLE_Authority)
	{
		RSv_StartCrafting(Recipe, WorkingStation);
		return true; // Assume success, server will validate
	}

	// Server-side logic
	if (!Sv_CanCraft(Recipe, WorkingStation))
	{
		return false;
	}

	// Setup crafting operation
	FActiveCraftingOperation NewOperation;
	NewOperation.Recipe = Recipe;
	if (WorkingStation && WorkingStation->Implements<UWorkingStationInterface>())
	{
		NewOperation.WorkingStation.SetObject(WorkingStation);
		NewOperation.WorkingStation.SetInterface(Cast<IWorkingStationInterface>(WorkingStation));
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	NewOperation.StartTime = CurrentTime;
	NewOperation.CompletionTime = CurrentTime + Recipe->CraftingTime;

	ActiveCraftingOperation = NewOperation;

	// Notify working station
	if (NewOperation.WorkingStation.GetInterface())
	{
		IWorkingStationInterface::Execute_OnCraftingStarted(
			NewOperation.WorkingStation.GetObject(),
			Recipe,
			GetOwner()
		);
	}

	// Fire event
	OnCraftingStarted.Broadcast(NewOperation);

	UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::StartCrafting - Started crafting: %s"), *Recipe->RecipeName.ToString());
	return true;
}

void UCraftingComponent::CancelCrafting()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		RSv_CancelCrafting();
		return;
	}

	if (!ActiveCraftingOperation.IsValid())
	{
		return;
	}

	UCraftingRecipeDataAsset* Recipe = ActiveCraftingOperation.Recipe;

	// Notify working station
	if (ActiveCraftingOperation.WorkingStation.GetInterface())
	{
		IWorkingStationInterface::Execute_OnCraftingCancelled(
			ActiveCraftingOperation.WorkingStation.GetObject(),
			Recipe,
			GetOwner()
		);
	}

	// Clear operation
	ActiveCraftingOperation = FActiveCraftingOperation();

	// Fire event
	OnCraftingCancelled.Broadcast(Recipe);

	UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::CancelCrafting - Cancelled crafting: %s"), *Recipe->RecipeName.ToString());
}

float UCraftingComponent::GetCraftingProgress() const
{
	if (!ActiveCraftingOperation.IsValid())
	{
		return 0.0f;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	return ActiveCraftingOperation.GetProgress(CurrentTime);
}

// ===== Server RPCs =====

void UCraftingComponent::RSv_StartCrafting_Implementation(UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation)
{
	StartCrafting(Recipe, WorkingStation);
}

void UCraftingComponent::RSv_CancelCrafting_Implementation()
{
	CancelCrafting();
}

// ===== Server Implementation =====

bool UCraftingComponent::Sv_CanCraft(const UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation) const
{
	if (!Recipe)
	{
		return false;
	}

	// Check if already crafting
	if (ActiveCraftingOperation.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Already crafting"));
		return false;
	}

	// Check if recipe is known
	if (!HasRecipe(Recipe))
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Recipe not known: %s"), *Recipe->RecipeName.ToString());
		return false;
	}

	// Check if working station is required
	if (Recipe->RequiresWorkingStation())
	{
		if (!WorkingStation)
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Working station required but not provided"));
			return false;
		}

		if (!WorkingStation->Implements<UWorkingStationInterface>())
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Working station does not implement interface"));
			return false;
		}

		IWorkingStationInterface* StationInterface = Cast<IWorkingStationInterface>(WorkingStation);
		if (!StationInterface)
		{
			return false;
		}

		// Check station type matches
		const FWorkingStationType StationType = IWorkingStationInterface::Execute_GetWorkingStationType(WorkingStation);
		if (StationType.StationType != Recipe->RequiredWorkingStation.StationType)
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Working station type mismatch"));
			return false;
		}

		// Check if station is operational
		if (!IWorkingStationInterface::Execute_IsOperational(WorkingStation))
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingComponent::Sv_CanCraft - Working station not operational"));
			return false;
		}
	}

	// TODO: Check if player has required ingredients in inventory

	return true;
}

void UCraftingComponent::Sv_CompleteCrafting()
{
	if (!ActiveCraftingOperation.IsValid())
	{
		return;
	}

	UCraftingRecipeDataAsset* Recipe = ActiveCraftingOperation.Recipe;

	// TODO: Consume ingredients from inventory
	// TODO: Add output items to inventory

	// Notify working station
	if (ActiveCraftingOperation.WorkingStation.GetInterface())
	{
		IWorkingStationInterface::Execute_OnCraftingCompleted(
			ActiveCraftingOperation.WorkingStation.GetObject(),
			Recipe,
			GetOwner()
		);
	}

	// Clear operation
	ActiveCraftingOperation = FActiveCraftingOperation();

	// Fire event
	OnCraftingCompleted.Broadcast(Recipe, true);

	UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::Sv_CompleteCrafting - Completed crafting: %s"), *Recipe->RecipeName.ToString());
}

void UCraftingComponent::Sv_TickCrafting(float DeltaTime)
{
	if (!ActiveCraftingOperation.IsValid())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= ActiveCraftingOperation.CompletionTime)
	{
		Sv_CompleteCrafting();
	}
}

// ===== Client Handlers =====

void UCraftingComponent::Handle_OnKnownRecipesChanged()
{
	// Client can react to recipe changes here if needed
	UE_LOG(LogTemp, Log, TEXT("UCraftingComponent::Handle_OnKnownRecipesChanged - Known recipes updated, count: %d"), KnownRecipes.Num());
}
