// Fill out your copyright notice in the Description page of Project Settings.


#include "EntityNode.h"
#include "EntityFacade.h"
#include "Engine/World.h"

void UEntityNode::BindToFacade(AEntityFacade* Facade)
{
	if (Facade)
	{
		BoundFacade = Facade;
		Facade->SetBackendNode(this);
	}
}

void UEntityNode::UnbindFromFacade()
{
	if (BoundFacade.IsValid())
	{
		BoundFacade->SetBackendNode(nullptr);
		BoundFacade.Reset();
	}
}

AEntityFacade* UEntityNode::GetBoundFacade() const
{
	return BoundFacade.Get();
}

bool UEntityNode::HasValidFacade() const
{
	return BoundFacade.IsValid();
}

void UEntityNode::TickNode(float DeltaTime)
{
	if (!HasValidFacade())
	{
		return;
	}

	TimeSinceLastLogicTick += DeltaTime;

	if (TimeSinceLastLogicTick >= LogicTickInterval)
	{
		// Run logic update
		ProcessLogic_Native(TimeSinceLastLogicTick);
		ProcessLogic(TimeSinceLastLogicTick);

		// Push updates to facade
		PushUpdateToFacade();

		TimeSinceLastLogicTick = 0.0f;
	}
}

void UEntityNode::PushUpdateToFacade()
{
	if (HasValidFacade())
	{
		BoundFacade->UpdateFromNode_Native(this);
		BoundFacade->UpdateFromNode(this);
	}
}

UWorld* UEntityNode::GetWorld() const
{
	if (HasValidFacade())
	{
		return BoundFacade->GetWorld();
	}
	return nullptr;
}