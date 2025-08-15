// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkLoadingGameRule.h"

#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/MainController.h"
#include "Bluevox/Game/WorldSave.h"

void UChunkLoadingGameRule::OnSetup(AGameManager* InGameManager)
{
	GameManager = InGameManager;
	
	GameManager->OnPlayerJoin.AddDynamic(this, &UChunkLoadingGameRule::OnPlayerJoin);
	GameManager->OnPlayerLeave.AddDynamic(this, &UChunkLoadingGameRule::OnPlayerLeave);

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const AMainController* PC = Cast<AMainController>(Iterator->Get());
		if (PC)
		{
			GameManager->VirtualMap->RegisterPlayer(PC);
		}
	}
}

void UChunkLoadingGameRule::OnPlayerJoin(AMainController* PlayerController)
{
	if (GameManager->bServer && GameManager->LocalController != PlayerController && GameManager->bInitialized)
	{
		GameManager->VirtualMap->RegisterPlayer(PlayerController);
	}
}

void UChunkLoadingGameRule::OnPlayerLeave(AMainController* PlayerController)
{
	GameManager->WorldSave->SavePlayer(PlayerController);
	GameManager->VirtualMap->UnregisterPlayer(PlayerController);
}
