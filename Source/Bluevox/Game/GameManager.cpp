// Fill out your copyright notice in the Description page of Project Settings.


#include "GameManager.h"

#include "MainCharacter.h"
#include "MainController.h"
#include "WorldSave.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/VirtualMap/ChunkTaskManager.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Tick/TickManager.h"
#include "Bluevox/Entity/EntityConversionSystem.h"
#include "GameRules/GameRule.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"


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

	ChunkTaskManager = NewObject<UChunkTaskManager>(this)->Init(this);
	VirtualMap = NewObject<UVirtualMap>(this, TEXT("VirtualMap"))->Init(this);

	ChunkRegistry = NewObject<UChunkRegistry>(this, TEXT("ChunkRegistry"))->Init(this);

	TickManager = NewObject<UTickManager>(this, TEXT("TickManager"))->Init();

	// Create entity conversion system on server only
	if (bServer)
	{
		EntityConversionSystem = NewObject<UEntityConversionSystem>(this, TEXT("EntityConversionSystem"))->Init(this);
	}
	
	const auto Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	LocalController = Cast<AMainController>(Controller);
	LocalPlayerId = LocalController ? LocalController->Id : 0;

	const auto PlayerState = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	LocalPlayerState = PlayerState;

	const auto Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	LocalCharacter = Cast<AMainCharacter>(Character);

	for (const auto& Rule : GameRules)
	{
		Rule->OnSetup(this);
	}

	for (const auto& Rule : GameRules)
	{
		Rule->PostSetup(this);
	}

	bInitialized = true;

	FPrimaryAssetTypeInfo Info;
	const bool bHasType = UAssetManager::Get().GetPrimaryAssetTypeInfo(FPrimaryAssetType("InstanceType"), Info);
	UE_LOG(LogTemp, Warning, TEXT("[AM] HasType=%d"), bHasType ? 1 : 0);
	TArray<FPrimaryAssetId> AllIds;
	UAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType("InstanceType"), AllIds);
	UE_LOG(LogTemp, Warning, TEXT("[AM] InstanceType IDs: %d"), AllIds.Num());
	for (const FPrimaryAssetId& Id : AllIds)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AM] ID: %s"), *Id.ToString());
	}
}

