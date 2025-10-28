// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "InstanceData.generated.h"

class UInstanceTypeDataAsset;

/**
 * Metadata for a single instance
 * Stores transform and reference to instance type data asset
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FInstanceData
{
	GENERATED_BODY()

	// Transform of the instance (local to chunk)
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	// Reference to the instance type data asset
	UPROPERTY()
	TSoftObjectPtr<UInstanceTypeDataAsset> InstanceType;

	FInstanceData() = default;

	FInstanceData(const FTransform& InTransform, const TSoftObjectPtr<UInstanceTypeDataAsset>& InInstanceType)
		: Transform(InTransform), InstanceType(InInstanceType)
	{
	}

	// Convenience constructor from separate components
	FInstanceData(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale,
	              const TSoftObjectPtr<UInstanceTypeDataAsset>& InInstanceType)
		: Transform(FTransform(InRotation, InLocation, InScale)), InstanceType(InInstanceType)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FInstanceData& Data)
	{
		Ar << Data.Transform;

		// Serialize asset path as string for better compatibility
		FString AssetPath;
		if (Ar.IsSaving())
		{
			AssetPath = Data.InstanceType.ToString();
		}

		Ar << AssetPath;

		if (Ar.IsLoading())
		{
			Data.InstanceType = TSoftObjectPtr<UInstanceTypeDataAsset>(FSoftObjectPath(AssetPath));
		}

		return Ar;
	}
};

/**
 * Collection of instances of a specific type within a chunk
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FInstanceCollection
{
	GENERATED_BODY()

	// Primary asset ID of the instance type
	UPROPERTY()
	FPrimaryAssetId InstanceTypeId;

	// Array of instances
	UPROPERTY()
	TArray<FInstanceData> Instances;

	// Server-only: indices of instances that have been converted to entities
	UPROPERTY(NotReplicated)
	TSet<int32> ConvertedInstanceIndices;

	// Cached transforms for efficient bulk rendering
	TArray<FTransform> CachedTransforms;

	// Flag to indicate if cached transforms need rebuilding
	bool bTransformsCacheDirty = true;

	FInstanceCollection() = default;

	explicit FInstanceCollection(const FPrimaryAssetId& InTypeId)
		: InstanceTypeId(InTypeId)
	{
	}

	void AddInstance(const FInstanceData& Instance)
	{
		Instances.Add(Instance);
		bTransformsCacheDirty = true;
	}

	void RemoveInstance(int32 Index)
	{
		if (Instances.IsValidIndex(Index))
		{
			Instances.RemoveAt(Index);
			ConvertedInstanceIndices.Remove(Index);
			bTransformsCacheDirty = true;
		}
	}

	// Rebuild cached transforms for bulk operations
	void RebuildTransformCache()
	{
		CachedTransforms.Empty(Instances.Num());
		for (const auto& Instance : Instances)
		{
			CachedTransforms.Add(Instance.Transform);
		}
		bTransformsCacheDirty = false;
	}

	// Get cached transforms, rebuilding if necessary
	const TArray<FTransform>& GetCachedTransforms()
	{
		if (bTransformsCacheDirty)
		{
			RebuildTransformCache();
		}
		return CachedTransforms;
	}

	friend FArchive& operator<<(FArchive& Ar, FInstanceCollection& Collection)
	{
		// Serialize asset ID as string
		FString AssetIdString;
		if (Ar.IsSaving())
		{
			AssetIdString = Collection.InstanceTypeId.ToString();
		}

		Ar << AssetIdString;

		if (Ar.IsLoading())
		{
			Collection.InstanceTypeId = FPrimaryAssetId(AssetIdString);
		}

		// Serialize instances
		Ar << Collection.Instances;

		// Don't serialize runtime data (ConvertedInstanceIndices, CachedTransforms)
		// These will be rebuilt at runtime

		return Ar;
	}
};
