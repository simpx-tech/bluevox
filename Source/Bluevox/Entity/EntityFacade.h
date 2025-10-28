// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EntityFacade.generated.h"

class UEntityNode;
struct FInstanceData;

/**
 * Base class for all entity facades - the replicated actor visible to clients
 * This is a generic base that can represent machines, animals, NPCs, etc.
 * Specific entity types should inherit from this class
 */
UCLASS(Abstract)
class BLUEVOX_API AEntityFacade : public AActor
{
	GENERATED_BODY()

protected:
	// Server-only reference to backend logic node
	UPROPERTY()
	UEntityNode* BackendNode = nullptr;

	// Store the source instance data (for respawning, etc.)
	FTransform SourceInstanceTransform;

public:
	AEntityFacade();

	// Called by the spawning system when this entity is created from an instance
	virtual void OnSpawnedFromInstance(const FInstanceData& SourceInstance);

	// Called by the backend node to update facade state (server only)
	UFUNCTION(BlueprintImplementableEvent, Category = "Entity")
	void UpdateFromNode(UEntityNode* Node);

	// C++ version that can be overridden
	virtual void UpdateFromNode_Native(UEntityNode* Node) { }

	// Set the backend node (server only)
	void SetBackendNode(UEntityNode* Node);

	// Get the backend node (server only, returns nullptr on clients)
	UFUNCTION(BlueprintPure, Category = "Entity")
	UEntityNode* GetBackendNode() const;

	// Check if this is running on server
	UFUNCTION(BlueprintPure, Category = "Entity")
	bool IsServer() const;

	// Get the source instance transform
	UFUNCTION(BlueprintPure, Category = "Entity")
	FTransform GetSourceInstanceTransform() const { return SourceInstanceTransform; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};