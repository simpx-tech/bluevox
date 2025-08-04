// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameMode.h"

#include "GameManager.h"
#include "MainCharacter.h"
#include "MainController.h"
#include "WorldSave.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Kismet/GameplayStatics.h"

AMainGameMode::AMainGameMode()
{
	PlayerControllerClass = AMainController::StaticClass();
	DefaultPawnClass = AMainCharacter::StaticClass();
}

void AMainGameMode::BeginPlay()
{
	Super::BeginPlay();

	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));
}

void AMainGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const auto MainController = Cast<AMainController>(Exiting);
	const auto MainCharacter = Cast<AMainCharacter>(MainController->GetPawn());

	if (MainCharacter)
	{
		MainController->SavedGlobalPosition = FGlobalPosition::FromActorLocation(MainCharacter->GetActorLocation());
	} else
	{
		MainController->SavedGlobalPosition = FGlobalPosition{0,0,0};
	}

	GameManager->WorldSave->SavePlayer(MainController);
	GameManager->VirtualMap->UnregisterPlayer(MainController);
}

