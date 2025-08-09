// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

class AGameManager;

/**
 * 
 */
UCLASS()
class BLUEVOX_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

	AMainGameMode();

	UPROPERTY(EditAnywhere);
	AGameManager* GameManager = nullptr;

	UPROPERTY(EditAnywhere)
	uint16 LastPlayerId = 0;
	
	virtual void BeginPlay() override;

	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting) override;
};
