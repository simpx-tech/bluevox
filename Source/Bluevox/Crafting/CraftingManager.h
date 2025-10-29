// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CraftingRecipeDataAsset.h"
#include "CraftingManager.generated.h"

/**
 * Global crafting manager subsystem
 * Manages all available recipes in the game and provides query functionality
 * This is a central registry for all crafting recipes
 */
UCLASS()
class BLUEVOX_API UCraftingManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== Recipe Registration =====

	/**
	 * Register a recipe with the manager
	 * @param Recipe The recipe to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void RegisterRecipe(UCraftingRecipeDataAsset* Recipe);

	/**
	 * Unregister a recipe from the manager
	 * @param Recipe The recipe to unregister
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void UnregisterRecipe(UCraftingRecipeDataAsset* Recipe);

	/**
	 * Register multiple recipes at once
	 * @param Recipes Array of recipes to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void RegisterRecipes(const TArray<UCraftingRecipeDataAsset*>& Recipes);

	/**
	 * Load and register all recipes from the asset manager
	 * This will search for all CraftingRecipe primary assets
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void LoadAllRecipes();

	// ===== Recipe Query =====

	/**
	 * Get a recipe by its ID
	 * @param RecipeId The recipe identifier
	 * @return The recipe, or null if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	UCraftingRecipeDataAsset* GetRecipeById(FName RecipeId) const;

	/**
	 * Get all registered recipes
	 * @return Array of all recipes
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> GetAllRecipes() const;

	/**
	 * Get all recipes for a specific working station
	 * @param StationType The working station type to filter by
	 * @return Array of matching recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> GetRecipesByWorkingStation(FWorkingStationType StationType) const;

	/**
	 * Get all recipes in a specific category
	 * @param Category The category to filter by
	 * @return Array of matching recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> GetRecipesByCategory(FName Category) const;

	/**
	 * Get all recipes that don't require a working station (hand-craftable)
	 * @return Array of hand-craftable recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> GetHandCraftableRecipes() const;

	/**
	 * Get all default-unlocked recipes
	 * @return Array of recipes unlocked by default
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> GetDefaultRecipes() const;

	/**
	 * Get count of registered recipes
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetRecipeCount() const { return RegisteredRecipes.Num(); }

	/**
	 * Check if a recipe is registered
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsRecipeRegistered(const UCraftingRecipeDataAsset* Recipe) const;

	// ===== Utility =====

	/**
	 * Get all unique working station types across all recipes
	 * @return Array of working station types
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FWorkingStationType> GetAllWorkingStationTypes() const;

	/**
	 * Get all unique categories across all recipes
	 * @return Array of category names
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FName> GetAllCategories() const;

private:
	/** All registered recipes, indexed by recipe ID */
	UPROPERTY()
	TMap<FName, UCraftingRecipeDataAsset*> RegisteredRecipes;

	/** Cached array of all recipes for quick iteration */
	UPROPERTY()
	TArray<UCraftingRecipeDataAsset*> RecipeArray;

	void RebuildRecipeArray();
};
