// Fill out your copyright notice in the Description page of Project Settings.


#include "MainController.h"

#include "GameManager.h"
#include "MainCharacter.h"
#include "Bluevox/Chunk/ChunkRegistry.h"
#include "Bluevox/Chunk/Data/ChunkData.h"
#include "Bluevox/Chunk/VirtualMap/VirtualMap.h"
#include "Bluevox/Network/PlayerNetwork.h"
#include "Bluevox/Network/UpdateChunkNetworkPacket.h"
#include "Bluevox/Shape/ShapeRegistry.h"
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

FBlockRaycastResult AMainController::BlockRaycast() const
{
	FVector ViewLoc; FRotator ViewRot;
	GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector Dir = ViewRot.Vector();
	const FVector Start = ViewLoc;
	const FVector End = Start + Dir * GameConstants::Distances::InteractionDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VoxelPick), true);
	Params.bReturnPhysicalMaterial = false;
	Params.AddIgnoredActor(GetPawn());

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		return FBlockRaycastResult();

	// Snap normal to the principal axis
	const FVector N = Hit.ImpactNormal;
	const int32 Axis = FMath::Abs(N.X) > FMath::Abs(N.Y)
		? (FMath::Abs(N.X) > FMath::Abs(N.Z) ? 0 : 2)
		: FMath::Abs(N.Y) > FMath::Abs(N.Z) ? 1 : 2;
	FVector Face(0,0,0);
	if (Axis == 0) Face.X = N.X >= 0 ? 1 : -1;
	if (Axis == 1) Face.Y = N.Y >= 0 ? 1 : -1;
	if (Axis == 2) Face.Z = N.Z >= 0 ? 1 : -1;

	// DrawDebugLine(
	// 	GetWorld(), Start, Hit.ImpactPoint, FColor::Yellow, false, -1.0f, 0, 1.0f);
	// DrawDebugLine(
	// 	GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Face * GameRules::Scaling::ZSize, FColor::Red, false, -1.0f, 0);
	
	const double Increment = 0.25 * (Axis == 2 ? GameConstants::Scaling::ZSize : GameConstants::Scaling::XYWorldSize);
	
	FBlockRaycastResult Result;
	Result.bHit = true;
	Result.Position = FGlobalPosition::FromActorLocation(Hit.ImpactPoint - Face * Increment);
	Result.PlacePosition = FGlobalPosition::FromActorLocation(Hit.ImpactPoint + Face * Increment);

	return Result;
}

void AMainController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const auto ActorPosition = FGlobalPosition::FromActorLocation(GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector);
	GEngine->AddOnScreenDebugMessage(
		0, 0.0f, FColor::Blue,
		FString::Printf(TEXT("Position: %s"), *ActorPosition.ToString()));
	
	LastRaycastResult = BlockRaycast();
	if (LastRaycastResult.bHit)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Green,
			FString::Printf(TEXT("Looking at block: %s"), *LastRaycastResult.Position.ToString()));
		GEngine->AddOnScreenDebugMessage(
			2, 0.0f, FColor::Blue,
			FString::Printf(TEXT("Place position: %s"), *LastRaycastResult.PlacePosition.ToString()));

		DrawDebugBox(
			GetWorld(), LastRaycastResult.Position.AsActorLocationCopy() + FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZSize * 0.5f),
			FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZSize * 0.5f),
			FQuat::Identity, FColor::Green, false, -1.0f);
		DrawDebugBox(
			GetWorld(), LastRaycastResult.PlacePosition.AsActorLocationCopy() + FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZSize * 0.5f),
			FVector(GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::XYWorldSize * 0.5f, GameConstants::Scaling::ZSize * 0.5f),
			FQuat::Identity, FColor::Blue, false, -1.0f);
	}
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

	GameManager->OnPlayerJoin.Broadcast(this);
}

void AMainController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (PlayerState && GameManager->bInitialized && GameManager->bClient)
	{
		PlayerNetwork->NotifyClientNetReady();
	}
}

void AMainController::Sv_LeftClick_Implementation()
{
	if (LastRaycastResult.bHit)
	{
		const auto UpdateChunkPacket = NewObject<UUpdateChunkNetworkPacket>(this)->Init(LastRaycastResult.Position, FPiece());
		
		// Run locally
		UpdateChunkPacket->OnReceive(GameManager);
		
		// TODO improve this
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (const AMainController* MainController = Cast<AMainController>(PlayerController))
			{
				if (MainController != GameManager->LocalController)
				{
					MainController->PlayerNetwork->SendToClient(UpdateChunkPacket);
				}
			}
		}
	}
}

void AMainController::Sv_RightClick_Implementation()
{
	if (LastRaycastResult.bHit)
	{
		const auto DirtShapeId = GameManager->ShapeRegistry->GetShapeIdByName(GameConstants::Constants::GShape_Layer_Dirt);
		
		const auto UpdateChunkPacket = NewObject<UUpdateChunkNetworkPacket>(this)->Init(LastRaycastResult.PlacePosition, FPiece(DirtShapeId, 1));
		
		// Run locally
		UpdateChunkPacket->OnReceive(GameManager);
		
		// TODO improve this
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (const AMainController* MainController = Cast<AMainController>(PlayerController))
			{
				if (MainController != GameManager->LocalController)
				{
					MainController->PlayerNetwork->SendToClient(UpdateChunkPacket);
				}
			}
		}
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
