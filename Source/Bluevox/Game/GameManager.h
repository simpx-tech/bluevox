// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameManager.generated.h"

class UChunkTaskManager;
class UGameRule;
class UWorldGenerator;
class UMaterialRegistry;
class UWorldSave;
class UTickManager;
class UChunkRegistry;
class AMainCharacter;
class AMainController;
class UVirtualMap;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoin, AMainController*, PlayerController);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeave, AMainController*, PlayerController);

UCLASS(BlueprintType)
class BLUEVOX_API AGameManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameManager();

	UFUNCTION()
	void OnBeginWorldTearDown(UWorld* World);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerJoin OnPlayerJoin;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerLeave OnPlayerLeave;
	
	UPROPERTY(EditAnywhere, Instanced, meta = (ShowOnlyInnerProperties), Category = "Game Rules")
	TArray<UGameRule*> GameRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	UStaticMesh* TreeMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	UMaterial* ChunkMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	UMaterial* ChunkMaterialTransparent = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool bInitialized = false;
	
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

	// Check if running as client-only
	UFUNCTION(BlueprintPure, Category = "Game")
	bool IsClientOnly() const { return bClientOnly; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UVirtualMap* VirtualMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UChunkRegistry* ChunkRegistry = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	UChunkTaskManager* ChunkTaskManager = nullptr;

	UPROPERTY(EditAnywhere, Category = "Game")
	UTickManager* TickManager = nullptr;

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
