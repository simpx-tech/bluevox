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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity Conversion", meta = (EditCondition = "bCanConvertToEntity"))
	float EntityConversionDistance = 500.0f;

	// TODO move this to the generator not the asset
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

	// Get the primary asset ID for this data asset
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static void EstimateInstanceSpacingFromMesh(
		const UStaticMesh* Mesh,
		float ScaleToUseXY, float ScaleToUseZ,
		float XYBlockSize, float ZBlockSize,
		/*out*/ float& OutMinSpacingBlocks,
		/*out*/ int32& OutRequiredVoidBlocks,
		int32 ExtraXYBlockPadding = 0
	)
	{
		OutMinSpacingBlocks = 1.0f;
		OutRequiredVoidBlocks = 1;

		if (!Mesh) return;

		const FBox Box = Mesh->GetBoundingBox();
		const FVector Ext = Box.GetExtent(); // half sizes (cm)

		// World-space extents with chosen scale(s)
		const float widthXY_cm = 2.0f * FMath::Max(Ext.X, Ext.Y) * ScaleToUseXY;
		const float heightZ_cm  = 2.0f * Ext.Z * ScaleToUseZ;

		// Convert to blocks
		float minSpacingBlocks = widthXY_cm / FMath::Max(1e-3f, XYBlockSize);
		int32 requiredVoidBlocks = FMath::CeilToInt(heightZ_cm / FMath::Max(1e-3f, ZBlockSize));

		// Padding and clamping to your UPROPERTY ranges
		minSpacingBlocks = FMath::CeilToFloat(minSpacingBlocks) + ExtraXYBlockPadding;
		minSpacingBlocks = FMath::Clamp(minSpacingBlocks, 1.0f, 20.0f);
		requiredVoidBlocks = FMath::Clamp(requiredVoidBlocks, 1, 50);

		OutMinSpacingBlocks = minSpacingBlocks;
		OutRequiredVoidBlocks = requiredVoidBlocks;
	}
};