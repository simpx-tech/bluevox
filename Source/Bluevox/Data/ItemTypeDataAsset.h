// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemTypeDataAsset.generated.h"

/**
 * Defines stack amount thresholds for mesh selection
 */
USTRUCT(BlueprintType)
struct FStackMeshThreshold
{
	GENERATED_BODY()

	/** Minimum stack amount for this mesh (inclusive) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack Mesh", meta = (ClampMin = "1"))
	int32 MinStackAmount = 1;

	/** The mesh to use when stack amount >= MinStackAmount */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack Mesh")
	UStaticMesh* Mesh = nullptr;

	FStackMeshThreshold()
		: MinStackAmount(1), Mesh(nullptr)
	{
	}

	FStackMeshThreshold(int32 InMinAmount, UStaticMesh* InMesh)
		: MinStackAmount(InMinAmount), Mesh(InMesh)
	{
	}
};

/**
 * Data asset defining an item type configuration
 * Configurable in editor, used for both inventory and world representation
 */
UCLASS(BlueprintType)
class BLUEVOX_API UItemTypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Display name shown in UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Info")
	FText DisplayName;

	/** Description shown in UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Info", meta = (MultiLine = true))
	FText Description;

	/** Weight per single item */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** Icon texture for UI display */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	UTexture2D* Icon = nullptr;

	/**
	 * Stack-based meshes for world representation
	 * Sorted by MinStackAmount in ascending order
	 * Example: {1, SmallPile}, {10, MediumPile}, {50, LargePile}
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (TitleProperty = "MinStackAmount"))
	TArray<FStackMeshThreshold> StackMeshes;

	/**
	 * Get the appropriate mesh for a given stack amount
	 * Returns the mesh with the highest threshold that doesn't exceed the stack amount
	 */
	UFUNCTION(BlueprintPure, Category = "Item")
	UStaticMesh* GetMeshForStackAmount(int32 StackAmount) const;

	/**
	 * Get the total weight for a given stack amount
	 */
	UFUNCTION(BlueprintPure, Category = "Item")
	float GetWeightForAmount(int32 Amount) const
	{
		return Weight * Amount;
	}

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("ItemType", GetFName());
	}
};
