// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameManager.generated.h"

class UTickManager;
class UChunkRegistry;
class AMainCharacter;
class AMainController;
class UVirtualMap;

UCLASS()
class BLUEVOX_API AGameManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bStandalone = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bServer = false;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bClient = false;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bDedicatedServer = false;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bClientOnly = false;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	UVirtualMap* VirtualMap = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	UChunkRegistry* ChunkRegistry = nullptr;

	UPROPERTY()
	UTickManager* TickManager = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	AMainController* LocalController = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	APlayerState* LocalPlayerState = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	AMainCharacter* LocalCharacter = nullptr;
};
