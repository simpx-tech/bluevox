#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Tick/GameTickable.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "UObject/Object.h"
#include "EntityConversionSystem.generated.h"

class AGameManager;
class AEntityFacade;
struct FEntityRecord;

// Key to track converted entities: ChunkPosition + EntityArrayIndex
USTRUCT()
struct FConvertedEntityKey
{
	GENERATED_BODY()

	UPROPERTY()
	FChunkPosition ChunkPosition;

	UPROPERTY()
	int32 EntityArrayIndex = INDEX_NONE;

	FConvertedEntityKey() = default;
	FConvertedEntityKey(const FChunkPosition& InChunk, int32 InIndex)
		: ChunkPosition(InChunk), EntityArrayIndex(InIndex) {}

	bool operator==(const FConvertedEntityKey& Other) const
	{
		return ChunkPosition == Other.ChunkPosition && EntityArrayIndex == Other.EntityArrayIndex;
	}

	friend uint32 GetTypeHash(const FConvertedEntityKey& Key)
	{
		return HashCombine(GetTypeHash(Key.ChunkPosition), GetTypeHash(Key.EntityArrayIndex));
	}
};

UCLASS()
class BLUEVOX_API UEntityConversionSystem : public UObject, public IGameTickable
{
	GENERATED_BODY()
public:
	UEntityConversionSystem() = default;

	UFUNCTION()
	UEntityConversionSystem* Init(AGameManager* InGameManager);

	// IGameTickable
	virtual void GameTick(float DeltaTime) override;

private:
	UPROPERTY()
	AGameManager* GameManager = nullptr;

	// Track which entities are currently converted to facades
	UPROPERTY()
	TMap<FConvertedEntityKey, TWeakObjectPtr<AEntityFacade>> ConvertedEntities;

	float AccumulatedSeconds = 0.0f;

	void EvaluateConversions();

	// Helper to get chunks within range of player positions
	TArray<FChunkPosition> GetChunksWithinRange(const TArray<FVector>& PlayerLocations, float RangeCm) const;

	// Convert specific entities from instances to facades
	void ConvertEntitiesToFacades(class AChunk* Chunk, const TArray<int32>& EntityIndices);

	// Convert specific facades back to instances
	void ConvertFacadesToInstances(const TArray<FConvertedEntityKey>& KeysToRevert);
};