#include "EntityNode.h"

#include "EntityFacade.h"

UEntityNode* UEntityNode::Init(AEntityFacade* InFacade)
{
	Facade = InFacade;
	return this;
}

void UEntityNode::OnServerTick(float DeltaSeconds)
{
	// TODO: server-side processing for this entity goes here
}

void UEntityNode::OnFacadeStateChanged()
{
	// TODO: react to facade replicated state changes if needed
}
