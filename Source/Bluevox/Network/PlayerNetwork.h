// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PendingReceive.h"
#include "PendingSend.h"
#include "Components/ActorComponent.h"
#include "PlayerNetwork.generated.h"

USTRUCT()
struct FPendingExecution
{
	GENERATED_BODY()

	FPendingExecution()
	{
	}

	FPendingExecution(const uint32 InPacketId, UNetworkPacket* InPacket)
		: PacketId(InPacketId), Packet(InPacket)
	{
	}

	bool operator<(const FPendingExecution& Other) const
	{
		return PacketId < Other.PacketId;
	}

	UPROPERTY()
	uint32 PacketId = 0;

	UPROPERTY()
	UNetworkPacket* Packet = nullptr;
};

USTRUCT()
struct FPendingResend
{
	GENERATED_BODY()

	FPendingResend()
	{
	}

	FPendingResend(const uint32 InPacketId, const TArray<int32>& InIndexes)
		: PacketId(InPacketId), Indexes(InIndexes)
	{
	}

	UPROPERTY()
	uint32 PacketId = 0;

	UPROPERTY()
	int32 LastSentIndex = -1;

	UPROPERTY()
	TArray<int32> Indexes;
};

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
	uint32 ChunkSize = 1024; // 1 KB

	// TODO erase this over time, so we don't accumulate too many unknown chunks in memory
	TMap<uint32, TArray<FPacketChunk>> UnknownChunks;
	
	UPROPERTY(EditAnywhere)
	uint32 MaxSentBytesPerSecond = 1024 * 512; // 512 kb/s

	UPROPERTY(EditAnywhere)
	int32 ResendTimeoutSecs = 5;
	
	UPROPERTY()
	uint32 SentThisSecond = 0;

	UPROPERTY()
	uint32 LastResetTime = 0;

	UPROPERTY()
	TArray<FPendingExecution> PendingExecution;

	UPROPERTY()
	TArray<FPendingSend> PlayerNotReadyWaitingSends;
	
	UPROPERTY()
	uint32 ExpectedPacketId = 0;

	UPROPERTY()
	bool bClientNetReady = false;

	UFUNCTION()
	void TryToRunPacket(uint32 PacketId, UNetworkPacket* Packet);
	
	UFUNCTION(Server, Reliable)
	void Sv_SendPacketHeader(const FPacketHeader& PacketHeader);

	UFUNCTION(Client, Reliable)
	void Cl_SendPacketHeader(const FPacketHeader& PacketHeader);

	UFUNCTION()
	inline void HandlePacketHeader(const FPacketHeader& PacketHeader);
	
	UFUNCTION()
	bool ProcessPendingSend(FPendingSend& PendingSend);

	UFUNCTION()
	bool ProcessPendingResend(FPendingResend& PendingResend);
	
	UFUNCTION(Client, Unreliable)
	void Cl_ReceiveServerChunk(const FPacketChunk& PacketChunk);

	UFUNCTION(Server, Unreliable)
	void Sv_ReceiveClientChunk(const FPacketChunk& PacketChunk);

	UFUNCTION()
	void HandlePacketChunk(const FPacketChunk& PacketChunk);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AGameManager* GameManager = nullptr;

	UFUNCTION()
	void UpdateMaxSentBytesPerSecond(const int32 NewMaxSentBytesPerSecond)
	{
		MaxSentBytesPerSecond = NewMaxSentBytesPerSecond;
	}
	
	// Sets default values for this component's properties
	UPlayerNetwork();

	UPlayerNetwork* Init(AGameManager* InGameManager, AMainController* InLocalController, APlayerState* InLocalPlayer);

	UFUNCTION(Client, Reliable)
	void Cl_ConfirmReceivedPacket(uint32 PacketId);

	UFUNCTION(Server, Reliable)
	void Sv_ConfirmReceivedPacket(uint32 PacketId);

	UFUNCTION(Client, Reliable)
	void Cl_AskForResend(uint32 PacketId, const TArray<int32>& Indexes);

	UFUNCTION(Server, Reliable)
	void Sv_AskForResend(uint32 PacketId, const TArray<int32>& Indexes);

	UFUNCTION(BlueprintCallable, Category="Network")
	void SendToServer(UNetworkPacket* Packet);

	UFUNCTION(BlueprintCallable, Category="Network")
	void SendToClient(UNetworkPacket* Packet);

	UFUNCTION(Category="Network")
	void HandleConfirmedPacket(uint32 PacketId);

	UFUNCTION(Server, Reliable)
	void NotifyClientNetReady();

	virtual bool IsSupportedForNetworking() const override;
	
	virtual bool IsNameStableForNetworking() const override;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	TArray<FPendingResend> PendingResends;
	
	UPROPERTY()
	TMap<uint32, FPendingSend> PendingSends;
	
	UPROPERTY()
	TMap<uint32, FPendingReceive> PendingReceives;
};
