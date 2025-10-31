#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "EntityTypes.generated.h"

/**
 * Lightweight persisted record of an entity that lives inside a chunk save.
 * - Transform is local to the chunk (same convention as instances)
 * - InstanceTypeId identifies the visual/asset type used for instancing when not converted
 * - CustomData can store arbitrary per-entity binary payloads (server authoritative)
 * - PersistentId is an ID intended to remain stable across save/load and network updates
 * - InstanceIndex is the index inside the instance collection for this type when rendered as instance
 */
USTRUCT(BlueprintType)
struct BLUEVOX_API FEntityRecord
{
	GENERATED_BODY()

	// Local-to-chunk transform
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	// Type of this entity (maps to UInstanceTypeDataAsset PrimaryAssetId)
	UPROPERTY()
	FPrimaryAssetId InstanceTypeId;

	UPROPERTY()
	TArray<uint8> CustomData;

	// When represented as an instance, which index in the collection this corresponds to
	UPROPERTY()
	int32 InstanceIndex = INDEX_NONE;

	// Persistent array index inside the chunk's TSparseArray (for stable mapping)
	UPROPERTY()
	int32 ArrayIndex = INDEX_NONE;

	// Runtime flags (not saved)
	UPROPERTY(Transient)
	bool bIsConvertedToEntity = false; // server-side runtime flag

	friend FArchive& operator<<(FArchive& Ar, FEntityRecord& Rec)
	{
		Ar << Rec.Transform;

		FString AssetIdString;
		if (Ar.IsSaving())
		{
			AssetIdString = Rec.InstanceTypeId.ToString();
		}
		Ar << AssetIdString;
		if (Ar.IsLoading())
		{
			Rec.InstanceTypeId = FPrimaryAssetId(AssetIdString);
		}

		Ar << Rec.CustomData;
		return Ar;
	}
};
