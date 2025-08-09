// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameManager.generated.h"

class UMaterialRegistry;
class UShapeRegistry;
class UWorldSave;
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
	UPROPERTY(EditAnywhere, Category = "Development")
	bool bEraseAllSavesOnStart = false;

	UPROPERTY(EditAnywhere, Category = "Development")
	bool bOverrideWorldGenerator = false;

	UPROPERTY(EditAnywhere, Category = "Development", meta = (EditCondition = "bOverrideWorldGenerator"))
	TSubclassOf<class UWorldGenerator> WorldGeneratorClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	UMaterial* ChunkMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bStandalone = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bServer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bClient = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bDedicatedServer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bClientOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UVirtualMap* VirtualMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UChunkRegistry* ChunkRegistry = nullptr;

	UPROPERTY(EditAnywhere, Category = "Game")
	UTickManager* TickManager = nullptr;

	UPROPERTY(EditAnywhere, Category = "Game")
	UShapeRegistry* ShapeRegistry = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UMaterialRegistry* MaterialRegistry = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	AMainController* LocalController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	APlayerState* LocalPlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	AMainCharacter* LocalCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category = "Game")
	uint16 LocalPlayerId = 0;
	
	UPROPERTY(EditAnywhere)
	UWorldSave* WorldSave = nullptr;
};
