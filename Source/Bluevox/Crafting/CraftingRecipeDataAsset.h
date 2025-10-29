// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipeDataAsset.generated.h"

/**
 * Represents a single item and its quantity in a recipe
 */
USTRUCT(BlueprintType)
struct FCraftingIngredient
{
	GENERATED_BODY()

	/** The item class or identifier (can be UInstanceTypeDataAsset or inventory item) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TSoftObjectPtr<UObject> Item;

	/** Quantity required */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	bool operator==(const FCraftingIngredient& Other) const
	{
		return Item == Other.Item && Quantity == Other.Quantity;
	}
};

/**
 * Identifies a working station type for recipes
 */
USTRUCT(BlueprintType)
struct FWorkingStationType
{
	GENERATED_BODY()

	/** Unique identifier for this working station type (e.g., "Furnace", "CraftingTable", "AnvilStation") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FName StationType;

	/** Display name for UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FText DisplayName;

	bool operator==(const FWorkingStationType& Other) const
	{
		return StationType == Other.StationType;
	}

	bool IsValid() const
	{
		return !StationType.IsNone();
	}
};

/**
 * Data asset defining a crafting recipe
 * Recipes are data-driven and can be registered/unregistered at runtime
 */
UCLASS(BlueprintType)
class BLUEVOX_API UCraftingRecipeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName RecipeId;

	/** Display name for the recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FText RecipeName;

	/** Description of what this recipe creates */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FText Description;

	/** Icon for UI display */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Items required to craft this recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftingIngredient> InputIngredients;

	/** Items produced by this recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftingIngredient> OutputItems;

	/** Working station required (empty if can craft without station) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FWorkingStationType RequiredWorkingStation;

	/** Time in seconds to craft this recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0.0"))
	float CraftingTime = 1.0f;

	/** Experience gained when crafting this recipe */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0"))
	int32 ExperienceReward = 0;

	/** Category for organization (e.g., "Weapons", "Tools", "Building") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Category;

	/** Whether this recipe is unlocked by default for all players */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	bool bUnlockedByDefault = false;

	/** Priority for sorting recipes (higher = shown first) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 Priority = 0;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("CraftingRecipe", RecipeId);
	}

	/** Check if this recipe requires a working station */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool RequiresWorkingStation() const
	{
		return RequiredWorkingStation.IsValid();
	}

	/** Get a string representation for debugging */
	FString ToString() const
	{
		return FString::Printf(TEXT("Recipe: %s (%s)"), *RecipeName.ToString(), *RecipeId.ToString());
	}
};
