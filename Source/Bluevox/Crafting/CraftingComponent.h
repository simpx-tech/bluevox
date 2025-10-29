// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingRecipeDataAsset.h"
#include "CraftingComponent.generated.h"

class IWorkingStationInterface;

/**
 * Result of a recipe query with pagination
 */
USTRUCT(BlueprintType)
struct FCraftingRecipeQueryResult
{
	GENERATED_BODY()

	/** Recipes in this page */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TArray<UCraftingRecipeDataAsset*> Recipes;

	/** Total number of recipes matching the query (across all pages) */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 TotalCount = 0;

	/** Current page number (0-indexed) */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 PageNumber = 0;

	/** Number of recipes per page */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 PageSize = 0;

	/** Whether there are more pages */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	bool bHasMorePages = false;
};

/**
 * Tracks the state of an active crafting operation
 */
USTRUCT(BlueprintType)
struct FActiveCraftingOperation
{
	GENERATED_BODY()

	/** The recipe being crafted */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	UCraftingRecipeDataAsset* Recipe = nullptr;

	/** Working station being used (can be null for hand crafting) */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TScriptInterface<IWorkingStationInterface> WorkingStation;

	/** Time when crafting started */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	float StartTime = 0.0f;

	/** Time when crafting will complete */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	float CompletionTime = 0.0f;

	bool IsValid() const
	{
		return Recipe != nullptr;
	}

	float GetProgress(float CurrentTime) const
	{
		if (!IsValid()) return 0.0f;
		const float Duration = CompletionTime - StartTime;
		if (Duration <= 0.0f) return 1.0f;
		return FMath::Clamp((CurrentTime - StartTime) / Duration, 0.0f, 1.0f);
	}
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeLearnedDelegate, UCraftingRecipeDataAsset*, Recipe, bool, bInitialLoad);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeForgottenDelegate, UCraftingRecipeDataAsset*, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftingStartedDelegate, const FActiveCraftingOperation&, Operation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingCompletedDelegate, UCraftingRecipeDataAsset*, Recipe, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftingCancelledDelegate, UCraftingRecipeDataAsset*, Recipe);

/**
 * Component that manages crafting for a player
 * Tracks known recipes, handles crafting requests, and manages active crafting operations
 * This component is replicated and server-authoritative
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLUEVOX_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ===== Recipe Management =====

	/**
	 * Learn a new recipe (server-authoritative)
	 * @param Recipe The recipe to learn
	 * @param bInitialLoad Whether this is during initial load (won't fire events)
	 * @return True if the recipe was learned (false if already known)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crafting")
	bool Sv_LearnRecipe(UCraftingRecipeDataAsset* Recipe, bool bInitialLoad = false);

	/**
	 * Forget a recipe (server-authoritative)
	 * @param Recipe The recipe to forget
	 * @return True if the recipe was forgotten (false if not known)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crafting")
	bool Sv_ForgetRecipe(UCraftingRecipeDataAsset* Recipe);

	/**
	 * Check if a recipe is known
	 * @param Recipe The recipe to check
	 * @return True if the player knows this recipe
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool HasRecipe(const UCraftingRecipeDataAsset* Recipe) const;

	/**
	 * Get all known recipes with pagination and optional filtering
	 * @param PageNumber The page to retrieve (0-indexed)
	 * @param PageSize Number of recipes per page
	 * @param WorkingStationFilter Optional filter by working station type
	 * @param CategoryFilter Optional filter by category
	 * @return Query result with recipes and pagination info
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	FCraftingRecipeQueryResult GetKnownRecipes(int32 PageNumber = 0, int32 PageSize = 20,
		FWorkingStationType WorkingStationFilter = FWorkingStationType(),
		FName CategoryFilter = NAME_None) const;

	/**
	 * Get count of all known recipes (with optional filtering)
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetKnownRecipeCount(FWorkingStationType WorkingStationFilter = FWorkingStationType(),
		FName CategoryFilter = NAME_None) const;

	// ===== Crafting Operations =====

	/**
	 * Start crafting a recipe (server RPC)
	 * @param Recipe The recipe to craft
	 * @param WorkingStation Optional working station to use
	 * @return True if crafting started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool StartCrafting(UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation = nullptr);

	/**
	 * Cancel the current crafting operation
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CancelCrafting();

	/**
	 * Get the current active crafting operation
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	FActiveCraftingOperation GetActiveCraftingOperation() const { return ActiveCraftingOperation; }

	/**
	 * Check if currently crafting
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsCrafting() const { return ActiveCraftingOperation.IsValid(); }

	/**
	 * Get current crafting progress (0-1)
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	float GetCraftingProgress() const;

	// ===== Events =====

	/** Fired when a recipe is learned */
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnRecipeLearnedDelegate OnRecipeLearned;

	/** Fired when a recipe is forgotten */
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnRecipeForgottenDelegate OnRecipeForgotten;

	/** Fired when crafting starts */
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingStartedDelegate OnCraftingStarted;

	/** Fired when crafting completes */
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingCompletedDelegate OnCraftingCompleted;

	/** Fired when crafting is cancelled */
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingCancelledDelegate OnCraftingCancelled;

protected:
	virtual void BeginPlay() override;

private:
	// ===== Server RPCs =====

	UFUNCTION(Server, Reliable)
	void RSv_StartCrafting(UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation);

	UFUNCTION(Server, Reliable)
	void RSv_CancelCrafting();

	// ===== Server Implementation =====

	bool Sv_CanCraft(const UCraftingRecipeDataAsset* Recipe, UObject* WorkingStation) const;
	void Sv_CompleteCrafting();
	void Sv_TickCrafting(float DeltaTime);

	// ===== Client Handlers =====

	UFUNCTION()
	void Handle_OnKnownRecipesChanged();

	// ===== Replicated Data =====

	/** Recipes this player knows (replicated) */
	UPROPERTY(Replicated, ReplicatedUsing = Handle_OnKnownRecipesChanged)
	TArray<UCraftingRecipeDataAsset*> KnownRecipes;

	/** Current active crafting operation (replicated) */
	UPROPERTY(Replicated)
	FActiveCraftingOperation ActiveCraftingOperation;
};
