// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EntityNode.generated.h"

class AEntityFacade;

/**
 * Base class for entity backend logic - server only
 * This contains the non-replicated logic that drives the entity behavior
 * Specific node types should inherit from this for different behaviors
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class BLUEVOX_API UEntityNode : public UObject
{
	GENERATED_BODY()

protected:
	// Weak reference to the facade this node controls
	UPROPERTY()
	TWeakObjectPtr<AEntityFacade> BoundFacade;

	// Time accumulator for logic ticks
	float TimeSinceLastLogicTick = 0.0f;

	// How often to run logic updates (in seconds)
	UPROPERTY(EditDefaultsOnly, Category = "Entity Node", meta = (ClampMin = "0.016", ClampMax = "1.0"))
	float LogicTickInterval = 0.1f;

public:
	// Bind this node to a facade
	virtual void BindToFacade(AEntityFacade* Facade);

	// Unbind from the current facade
	virtual void UnbindFromFacade();

	// Get the bound facade (may return nullptr)
	UFUNCTION(BlueprintPure, Category = "Entity Node")
	AEntityFacade* GetBoundFacade() const;

	// Check if we have a valid facade binding
	UFUNCTION(BlueprintPure, Category = "Entity Node")
	bool HasValidFacade() const;

	// Called to tick this node (called by a tick manager)
	virtual void TickNode(float DeltaTime);

	// Push current state to the facade (one-way communication)
	virtual void PushUpdateToFacade();

protected:
	// Override this to implement entity-specific logic
	UFUNCTION(BlueprintImplementableEvent, Category = "Entity Node")
	void ProcessLogic(float DeltaTime);

	// C++ version that can be overridden
	virtual void ProcessLogic_Native(float DeltaTime) { }

	// Helper to check if we're on server (always true for nodes)
	bool IsServer() const { return true; }

public:
	// Get world context (through facade if bound)
	virtual UWorld* GetWorld() const override;
};