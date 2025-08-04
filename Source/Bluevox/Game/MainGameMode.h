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

	virtual void BeginPlay() override;

	virtual void Logout(AController* Exiting) override;
};
