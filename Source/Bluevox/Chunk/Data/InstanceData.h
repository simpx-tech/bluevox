// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InstanceData.generated.h"

UENUM(BlueprintType)
enum class EInstanceType : uint8
{
	None UMETA(DisplayName = "None"),
	Tree UMETA(DisplayName = "Tree"),
	Rock UMETA(DisplayName = "Rock"),
	Bush UMETA(DisplayName = "Bush"),
};

/**
 * Metadata for a single instance
 * Stores location, rotation, scale, and extensible per-instance data
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FInstanceData
{
	GENERATED_BODY()

	// Local position within the chunk (X, Y in [0, ChunkSize), Z in [0, ChunkHeight))
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	// Rotation of the instance
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	// Scale of the instance
	UPROPERTY()
	FVector Scale = FVector::OneVector;

	// Optional metadata flags (for future use: interactability, health, etc.)
	UPROPERTY()
	uint32 Flags = 0;

	// Optional custom data (for future per-instance customization)
	UPROPERTY()
	TArray<float> CustomData;

	FInstanceData() = default;

	FInstanceData(const FVector& InLocation, const FRotator& InRotation = FRotator::ZeroRotator,
	              const FVector& InScale = FVector::OneVector)
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FInstanceData& Data)
	{
		Ar << Data.Location;
		Ar << Data.Rotation;
		Ar << Data.Scale;
		Ar << Data.Flags;
		Ar << Data.CustomData;
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

	UPROPERTY()
	EInstanceType InstanceType = EInstanceType::None;

	UPROPERTY()
	TArray<FInstanceData> Instances;

	FInstanceCollection() = default;

	explicit FInstanceCollection(EInstanceType InType)
		: InstanceType(InType)
	{
	}

	void AddInstance(const FInstanceData& Instance)
	{
		Instances.Add(Instance);
	}

	void RemoveInstance(int32 Index)
	{
		if (Instances.IsValidIndex(Index))
		{
			Instances.RemoveAt(Index);
		}
	}

	friend FArchive& operator<<(FArchive& Ar, FInstanceCollection& Collection)
	{
		uint8 TypeValue = static_cast<uint8>(Collection.InstanceType);
		Ar << TypeValue;

		if (Ar.IsLoading())
		{
			Collection.InstanceType = static_cast<EInstanceType>(TypeValue);
		}

		Ar << Collection.Instances;
		return Ar;
	}
};
