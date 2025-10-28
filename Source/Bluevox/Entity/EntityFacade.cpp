// Fill out your copyright notice in the Description page of Project Settings.


#include "EntityFacade.h"
#include "EntityNode.h"
#include "Net/UnrealNetwork.h"
#include "Bluevox/Chunk/Data/InstanceData.h"

AEntityFacade::AEntityFacade()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set this actor to replicate
	bReplicates = true;
	bAlwaysRelevant = false;
	NetCullDistanceSquared = FMath::Square(5000.0f);
}

void AEntityFacade::BeginPlay()
{
	Super::BeginPlay();
}

void AEntityFacade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up backend node if it exists
	if (BackendNode && IsServer())
	{
		BackendNode->MarkAsGarbage();
		BackendNode = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AEntityFacade::OnSpawnedFromInstance(const FInstanceData& SourceInstance)
{
	SourceInstanceTransform = SourceInstance.Transform;
}

void AEntityFacade::SetBackendNode(UEntityNode* Node)
{
	if (IsServer())
	{
		BackendNode = Node;
	}
}

UEntityNode* AEntityFacade::GetBackendNode() const
{
	return IsServer() ? BackendNode : nullptr;
}

bool AEntityFacade::IsServer() const
{
	return GetWorld() && (GetWorld()->GetNetMode() < NM_Client);
}