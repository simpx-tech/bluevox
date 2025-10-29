// Copyright Epic Games, Inc. All Rights Reserved.

#include "CraftingManager.h"
#include "Engine/AssetManager.h"

void UCraftingManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UCraftingManager::Initialize - Crafting manager initialized"));

	// Optionally auto-load recipes on initialization
	// LoadAllRecipes();
}

void UCraftingManager::Deinitialize()
{
	RegisteredRecipes.Empty();
	RecipeArray.Empty();

	Super::Deinitialize();
}

// ===== Recipe Registration =====

void UCraftingManager::RegisterRecipe(UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::RegisterRecipe - Recipe is null"));
		return;
	}

	if (Recipe->RecipeId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::RegisterRecipe - Recipe has invalid ID: %s"), *Recipe->GetName());
		return;
	}

	// Check for duplicate
	if (RegisteredRecipes.Contains(Recipe->RecipeId))
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::RegisterRecipe - Recipe already registered: %s"), *Recipe->RecipeId.ToString());
		return;
	}

	RegisteredRecipes.Add(Recipe->RecipeId, Recipe);
	RebuildRecipeArray();

	UE_LOG(LogTemp, Log, TEXT("UCraftingManager::RegisterRecipe - Registered recipe: %s (%s)"),
		*Recipe->RecipeName.ToString(), *Recipe->RecipeId.ToString());
}

void UCraftingManager::UnregisterRecipe(UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::UnregisterRecipe - Recipe is null"));
		return;
	}

	const int32 RemovedCount = RegisteredRecipes.Remove(Recipe->RecipeId);

	if (RemovedCount > 0)
	{
		RebuildRecipeArray();
		UE_LOG(LogTemp, Log, TEXT("UCraftingManager::UnregisterRecipe - Unregistered recipe: %s"), *Recipe->RecipeId.ToString());
	}
}

void UCraftingManager::RegisterRecipes(const TArray<UCraftingRecipeDataAsset*>& Recipes)
{
	for (UCraftingRecipeDataAsset* Recipe : Recipes)
	{
		if (!Recipe) continue;

		if (Recipe->RecipeId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::RegisterRecipes - Skipping recipe with invalid ID: %s"), *Recipe->GetName());
			continue;
		}

		if (!RegisteredRecipes.Contains(Recipe->RecipeId))
		{
			RegisteredRecipes.Add(Recipe->RecipeId, Recipe);
		}
	}

	RebuildRecipeArray();
	UE_LOG(LogTemp, Log, TEXT("UCraftingManager::RegisterRecipes - Registered %d recipes"), Recipes.Num());
}

void UCraftingManager::LoadAllRecipes()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	// Get all CraftingRecipe primary assets
	TArray<FPrimaryAssetId> RecipeAssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType("CraftingRecipe"), RecipeAssetIds);

	UE_LOG(LogTemp, Log, TEXT("UCraftingManager::LoadAllRecipes - Found %d recipe assets"), RecipeAssetIds.Num());

	// Load all recipes
	TArray<FName> Bundles;
	for (const FPrimaryAssetId& AssetId : RecipeAssetIds)
	{
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		UCraftingRecipeDataAsset* Recipe = Cast<UCraftingRecipeDataAsset>(AssetPath.TryLoad());

		if (Recipe)
		{
			RegisterRecipe(Recipe);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UCraftingManager::LoadAllRecipes - Failed to load recipe: %s"), *AssetId.ToString());
		}
	}
}

// ===== Recipe Query =====

UCraftingRecipeDataAsset* UCraftingManager::GetRecipeById(FName RecipeId) const
{
	UCraftingRecipeDataAsset* const* Recipe = RegisteredRecipes.Find(RecipeId);
	if (Recipe)
	{
		return *Recipe;
	}
	return nullptr;
}

TArray<UCraftingRecipeDataAsset*> UCraftingManager::GetAllRecipes() const
{
	return RecipeArray;
}

TArray<UCraftingRecipeDataAsset*> UCraftingManager::GetRecipesByWorkingStation(FWorkingStationType StationType) const
{
	TArray<UCraftingRecipeDataAsset*> Result;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe) continue;

		if (Recipe->RequiredWorkingStation.StationType == StationType.StationType)
		{
			Result.Add(Recipe);
		}
	}

	return Result;
}

TArray<UCraftingRecipeDataAsset*> UCraftingManager::GetRecipesByCategory(FName Category) const
{
	TArray<UCraftingRecipeDataAsset*> Result;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe) continue;

		if (Recipe->Category == Category)
		{
			Result.Add(Recipe);
		}
	}

	return Result;
}

TArray<UCraftingRecipeDataAsset*> UCraftingManager::GetHandCraftableRecipes() const
{
	TArray<UCraftingRecipeDataAsset*> Result;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe) continue;

		if (!Recipe->RequiresWorkingStation())
		{
			Result.Add(Recipe);
		}
	}

	return Result;
}

TArray<UCraftingRecipeDataAsset*> UCraftingManager::GetDefaultRecipes() const
{
	TArray<UCraftingRecipeDataAsset*> Result;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe) continue;

		if (Recipe->bUnlockedByDefault)
		{
			Result.Add(Recipe);
		}
	}

	return Result;
}

bool UCraftingManager::IsRecipeRegistered(const UCraftingRecipeDataAsset* Recipe) const
{
	if (!Recipe) return false;
	return RegisteredRecipes.Contains(Recipe->RecipeId);
}

// ===== Utility =====

TArray<FWorkingStationType> UCraftingManager::GetAllWorkingStationTypes() const
{
	TSet<FName> UniqueTypes;
	TArray<FWorkingStationType> Result;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe || !Recipe->RequiresWorkingStation()) continue;

		if (!UniqueTypes.Contains(Recipe->RequiredWorkingStation.StationType))
		{
			UniqueTypes.Add(Recipe->RequiredWorkingStation.StationType);
			Result.Add(Recipe->RequiredWorkingStation);
		}
	}

	return Result;
}

TArray<FName> UCraftingManager::GetAllCategories() const
{
	TSet<FName> UniqueCategories;

	for (UCraftingRecipeDataAsset* Recipe : RecipeArray)
	{
		if (!Recipe || Recipe->Category.IsNone()) continue;
		UniqueCategories.Add(Recipe->Category);
	}

	return UniqueCategories.Array();
}

// ===== Private =====

void UCraftingManager::RebuildRecipeArray()
{
	RecipeArray.Empty(RegisteredRecipes.Num());
	RegisteredRecipes.GenerateValueArray(RecipeArray);

	// Sort by priority (higher first), then by name
	RecipeArray.Sort([](const UCraftingRecipeDataAsset& A, const UCraftingRecipeDataAsset& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.RecipeName.CompareTo(B.RecipeName) < 0;
	});
}
