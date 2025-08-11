// Fill out your copyright notice in the Description page of Project Settings.


#include "GameManager.h"

#include "MainCharacter.h"
#include "MainController.h"
#include "WorldSave.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Generator/WorldGenerator.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Shape/ShapeRegistry.h"
#include "Bluevox/Tick/TickManager.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AGameManager::AGameManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AGameManager::OnBeginWorldTearDown(UWorld* World)
{
	WorldSave->Save();
}

// Called when the game starts or when spawned
void AGameManager::BeginPlay()
{
	Super::BeginPlay();

	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &AGameManager::OnBeginWorldTearDown);

	bServer = GetNetMode() == NM_ListenServer || GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone;
	bClient = GetNetMode() == NM_Client || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer;
	bClientOnly = GetNetMode() == NM_Client;
	bDedicatedServer = GetNetMode() == NM_DedicatedServer;
	bStandalone = GetNetMode() == NM_Standalone;

	VirtualMap = NewObject<UVirtualMap>(this, TEXT("VirtualMap"))->Init(this);
	
	ChunkRegistry = NewObject<UChunkRegistry>(this, TEXT("ChunkRegistry"))->Init(this);

	ShapeRegistry = NewObject<UShapeRegistry>(this, TEXT("ShapeRegistry"));
	ShapeRegistry->RegisterAll();
	
	TickManager = NewObject<UTickManager>(this, TEXT("TickManager"))->Init();
	
	const auto Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	LocalController = Cast<AMainController>(Controller);
	LocalPlayerId = LocalController ? LocalController->Id : 0;

	const auto PlayerState = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	LocalPlayerState = PlayerState;

	const auto Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	LocalCharacter = Cast<AMainCharacter>(Character);

	if (bEraseAllSavesOnStart)
	{
		IFileManager::Get().DeleteDirectory(*UWorldSave::GetWorldsDir(), false, true);
	}
	
	// TODO temp
	WorldSave = UWorldSave::CreateOrLoadWorldSave(this, "TestWorld", UFlatWorldGenerator::StaticClass());
	
	if (bOverrideWorldGenerator && WorldSave && WorldGeneratorOverride)
	{
		WorldSave->WorldGenerator = WorldGeneratorOverride->Init(this);
	}

	if (LocalController)
	{
		LocalController->GameManager = this;
		LocalController->PlayerNetwork->Init(this, LocalController, LocalPlayerState);
		// DEV DisableInput(this);

		if (bServer)
		{
			WorldSave->LoadPlayer(LocalController);

			if (bAutomaticallyRegisterPlayer)
			{
				VirtualMap->RegisterPlayer(LocalController);	
			}
		}	
	}
}

