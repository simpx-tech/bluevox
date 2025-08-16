// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSetupGameRule.h"

#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Game/WorldSave.h"
#include "Bluevox/Network/PlayerNetwork.h"

void UPlayerSetupGameRule::OnSetup(AGameManager* InGameManager)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMainController* PC = Cast<AMainController>(Iterator->Get());
		if (PC)
		{
			PC->GameManager = InGameManager;
			PC->PlayerNetwork->Init(InGameManager, PC, InGameManager->LocalPlayerState);

			if (InGameManager->bServer)
			{
				PC->SetActorEnableCollision(false);
				InGameManager->WorldSave->LoadPlayer(PC);
			}
		}
	}
}
