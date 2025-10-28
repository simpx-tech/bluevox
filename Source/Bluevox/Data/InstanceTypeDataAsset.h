// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Bluevox/Game/VoxelMaterial.h"
#include "InstanceTypeDataAsset.generated.h"

/**
 * Primary data asset that defines an instance type with all its properties
 * Used for data-driven instance configuration without code changes
 */
UCLASS(BlueprintType)
class BLUEVOX_API UInstanceTypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Visual properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	UStaticMesh* StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "100.0", ClampMax = "50000.0"))
	float CullDistance = 5000.0f;

	// Entity conversion settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity Conversion")
	bool bCanConvertToEntity = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity Conversion", meta = (EditCondition = "bCanConvertToEntity"))
	TSubclassOf<AActor> EntityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity Conversion", meta = (EditCondition = "bCanConvertToEntity", ClampMin = "100.0", ClampMax = "5000.0"))
	float EntityConversionDistance = 500.0f;

	// Generation rules
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float MinSpacing = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "1", ClampMax = "50"))
	int32 RequiredVoidSpace = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	TArray<EMaterial> ValidSurfaces;

	// Scale variation for generation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MaxScale = 1.2f;

public:
	// Get the primary asset ID for this data asset
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};