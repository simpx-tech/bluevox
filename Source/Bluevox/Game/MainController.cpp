// Fill out your copyright notice in the Description page of Project Settings.


#include "MainController.h"

#include "GameManager.h"
#include "MainCharacter.h"
#include "WorldSave.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AMainController::AMainController()
{
	PlayerNetwork = CreateDefaultSubobject<UPlayerNetwork>(TEXT("PlayerNetwork"));
}

void AMainController::HandleOnClientReadyChanged() const
{
	OnClientReadyChanged.Broadcast(bClientReady);
}

int32 AMainController::GetFarDistance() const
{
	return FarDistance;
}

void AMainController::SetFarDistance_Implementation(const int32 NewFarDistance)
{
	const auto OldFarDistance = FarDistance;
	FarDistance = NewFarDistance;
	GameManager->VirtualMap->Sv_UpdateFarDistanceForPlayer(this, OldFarDistance, FarDistance);
}

void AMainController::Sv_SetClientReady_Implementation(bool bReady)
{
	bClientReady = bReady;
}

void AMainController::BeginPlay()
{
	GameManager = Cast<AGameManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameManager::StaticClass()));
	PlayerNetwork->Init(GameManager, this, PlayerState);
	
	Super::BeginPlay();

	if (GameManager->bServer && GameManager->LocalController != this && GameManager->bInitialized)
	{
		GameManager->VirtualMap->RegisterPlayer(this);
	}
}

void AMainController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (PlayerState && GameManager->bInitialized && GameManager->bClient)
	{
		PlayerNetwork->NotifyClientNetReady();
	}
}

void AMainController::SetServerReady(const bool bReady)
{
	bServerReady = bReady;
	GameManager->LocalCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void AMainController::SetClientReady(const bool bReady)
{
	Sv_SetClientReady(bReady);
	// EnableInput(this);
}

void AMainController::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMainController, bServerReady);
	DOREPLIFETIME(AMainController, bClientReady);
}

void AMainController::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << SavedGlobalPosition;
}

void AMainController::SerializeForWorldSave(FArchive& Ar)
{
	Ar << SavedGlobalPosition;
}
