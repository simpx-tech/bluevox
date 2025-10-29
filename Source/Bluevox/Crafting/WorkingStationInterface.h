// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CraftingRecipeDataAsset.h"
#include "WorkingStationInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWorkingStationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that can serve as crafting stations
 * Implement this on actors or components that act as working stations
 */
class BLUEVOX_API IWorkingStationInterface
{
	GENERATED_BODY()

public:
	/**
	 * Get the type of this working station
	 * @return The station type identifier
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	FWorkingStationType GetWorkingStationType() const;

	/**
	 * Check if this working station is currently operational
	 * @return True if the station can be used for crafting
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	bool IsOperational() const;

	/**
	 * Called when crafting starts at this station
	 * @param Recipe The recipe being crafted
	 * @param Crafter The actor doing the crafting
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	void OnCraftingStarted(const UCraftingRecipeDataAsset* Recipe, AActor* Crafter);

	/**
	 * Called when crafting completes at this station
	 * @param Recipe The recipe that was crafted
	 * @param Crafter The actor who did the crafting
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	void OnCraftingCompleted(const UCraftingRecipeDataAsset* Recipe, AActor* Crafter);

	/**
	 * Called when crafting is cancelled at this station
	 * @param Recipe The recipe that was being crafted
	 * @param Crafter The actor who was crafting
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	void OnCraftingCancelled(const UCraftingRecipeDataAsset* Recipe, AActor* Crafter);
};
