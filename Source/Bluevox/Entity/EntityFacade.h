#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bluevox/Chunk/Position/ChunkPosition.h"
#include "EntityFacade.generated.h"

class UEntityNode;
class AGameManager;

UCLASS()
class BLUEVOX_API AEntityFacade : public AActor
{
	GENERATED_BODY()
public:
	AEntityFacade();

	UFUNCTION()
	AEntityFacade* Init(AGameManager* InGameManager);

	UPROPERTY(Replicated)
	FPrimaryAssetId InstanceTypeId;

	// Index inside the chunk entities sparse array
	UPROPERTY(Replicated)
	int32 EntityIndex = INDEX_NONE;

	// Chunk position this entity belongs to
	UPROPERTY(Replicated)
	FChunkPosition ChunkPosition;

	// Server-only backend node
	UPROPERTY(Transient)
	UEntityNode* Node = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostNetInit() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	UPROPERTY()
	AGameManager* GameManager = nullptr;
};