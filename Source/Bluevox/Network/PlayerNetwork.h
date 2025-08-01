// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PendingReceive.h"
#include "PendingSend.h"
#include "Components/ActorComponent.h"
#include "PlayerNetwork.generated.h"


struct FPacketChunk;
class AMainController;
class AGameManager;
class UNetworkPacket;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLUEVOX_API UPlayerNetwork : public UActorComponent
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	AMainController* LocalController = nullptr;

	UPROPERTY(EditAnywhere)
	APlayerState* LocalPlayerState = nullptr;

	UPROPERTY(EditAnywhere)
	bool bServer = false;
	
	UPROPERTY()
	uint32 PacketIdCounter = 0;

	UPROPERTY(EditAnywhere)
	uint32 ChunkSize = 1024;
	
	UPROPERTY(EditAnywhere)
	uint32 MaxSentBytesPerSecond = 1024 * 64; // 64 KB per second

	UPROPERTY()
	uint32 SentThisSecond = 0;

	UPROPERTY()
	uint32 LastResetTime = 0;
	
	UPROPERTY()
	uint32 LastCompletedPacketId = 0;

	UFUNCTION(Server, Reliable)
	void Sv_SendPacketHeader(const FPacketHeader& PacketHeader);

	UFUNCTION(Client, Reliable)
	void Cl_SendPacketHeader(const FPacketHeader& PacketHeader);

	UFUNCTION()
	inline void HandlePacketHeader(const FPacketHeader& PacketHeader);
	
	UFUNCTION()
	bool ProcessPendingSend(FPendingSend& PendingSend);

	UFUNCTION(Client, Reliable)
	void Cl_ReceiveClientChunk(const FPacketChunk& PacketChunk);

	UFUNCTION(Server, Reliable)
	void Sv_ReceiveServerChunk(const FPacketChunk& PacketChunk);

	UFUNCTION()
	void HandlePacketChunk(const FPacketChunk& PacketChunk);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AGameManager* GameManager = nullptr;
	
	// Sets default values for this component's properties
	UPlayerNetwork();

	UPlayerNetwork* Init(AGameManager* InGameManager, AMainController* InLocalController, APlayerState* InLocalPlayer);

	UFUNCTION(Client, Reliable)
	void Cl_ConfirmReceivedPacket(uint32 PacketId);

	UFUNCTION(Server, Reliable)
	void Sv_ConfirmReceivedPacket(uint32 PacketId);

	UFUNCTION(BlueprintCallable, Category="Network")
	void SendToServer(UNetworkPacket* Packet);

	UFUNCTION(BlueprintCallable, Category="Network")
	void SendToClient(UNetworkPacket* Packet);

	UFUNCTION(Category="Network")
	void HandleConfirmedPacket(uint32 PacketId);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	TMap<uint32, FPendingSend> PendingSends;
	
	UPROPERTY()
	TMap<uint32, FPendingReceive> PendingReceives;
};
