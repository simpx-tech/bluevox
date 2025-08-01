// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerNetwork.h"

#include "NetworkLogs.h"
#include "NetworkPacket.h"
#include "PacketChunk.h"
#include "ServerNetworkPacket.h"
#include "Bluevox/Game/GameManager.h"
#include "GameFramework/PlayerState.h"

void UPlayerNetwork::Sv_SendPacketHeader_Implementation(const FPacketHeader& PacketHeader)
{
	if (PacketHeader.PacketType->IsChildOf(UServerNetworkPacket::StaticClass()))
	{
		// Client to Server packet
		if (GameManager->bClientOnly)
		{
			UE_LOG(LogBluevoxNetwork, Warning, TEXT("Client tried to send server packet to server: PacketId=%u, PacketType=%s"), PacketHeader.PacketId, *PacketHeader.PacketType->GetName());
			return;
		}
	}
	
	HandlePacketHeader(PacketHeader);
}

void UPlayerNetwork::Cl_SendPacketHeader_Implementation(const FPacketHeader& PacketHeader)
{
	HandlePacketHeader(PacketHeader);
}

void UPlayerNetwork::HandlePacketHeader(const FPacketHeader& PacketHeader)
{
	UE_LOG(LogBluevoxNetwork, Log, TEXT("Received packet header: PacketId=%u, TotalSize=%d, PacketType=%s"),
		PacketHeader.PacketId, PacketHeader.TotalSize, PacketHeader.PacketType ? *PacketHeader.PacketType->GetName() : TEXT("None"));

	FPendingReceive PendingReceive(PacketHeader);
	PendingReceive.Data.SetNumUninitialized(PacketHeader.TotalSize);
	PendingReceive.MissingChunks = FMath::CeilToInt(static_cast<float>(PacketHeader.TotalSize / ChunkSize)) + 1;
	PendingReceive.ReceivedChunks.SetNum(PendingReceive.MissingChunks, false);
	PendingReceives.Add(PacketHeader.PacketId, MoveTemp(PendingReceive));
}

bool UPlayerNetwork::ProcessPendingSend(FPendingSend& PendingSend)
{
	if (PendingSend.bSentAll)
	{
		return true;
	}

	for (int32 Offset = PendingSend.Offset; Offset <= PendingSend.LastOffset; Offset += ChunkSize)
	{
		const auto SizeToCopy = FMath::Min(PendingSend.Data.Num() - PendingSend.Offset, static_cast<int32>(ChunkSize));
	
		FPacketChunk PacketChunk;
		PacketChunk.PacketId = PendingSend.PacketId;
		PacketChunk.Data.SetNumUninitialized(SizeToCopy);
		PacketChunk.Index = PendingSend.Offset / ChunkSize;
		FMemory::Memcpy(PacketChunk.Data.GetData(), PendingSend.Data.GetData() + PendingSend.Offset, SizeToCopy);

		PendingSend.Offset += SizeToCopy;
		SentThisSecond += SizeToCopy;
	
		if (bServer)
		{
			// From Server
			Cl_ReceiveClientChunk(PacketChunk);
		}
		else
		{
			// From Client
			Sv_ReceiveServerChunk(PacketChunk);
		}

		if (PendingSend.Offset >= PendingSend.LastOffset)
		{
			PendingSend.bSentAll = true;
		}

		if (SentThisSecond >= MaxSentBytesPerSecond)
		{
			return false;
		}
	}

	return true;
}

void UPlayerNetwork::Cl_ReceiveClientChunk_Implementation(const FPacketChunk& PacketChunk)
{
	HandlePacketChunk(PacketChunk);
}

void UPlayerNetwork::Sv_ReceiveServerChunk_Implementation(const FPacketChunk& PacketChunk)
{
	HandlePacketChunk(PacketChunk);
}

void UPlayerNetwork::HandlePacketChunk(const FPacketChunk& PacketChunk)
{
	UE_LOG(LogBluevoxNetwork, Log, TEXT("Received packet chunk: PacketId=%u, DataSize=%d, Index=%d"),
		PacketChunk.PacketId, PacketChunk.Data.Num(), PacketChunk.Index);
	
	const auto PendingReceive = PendingReceives.Find(PacketChunk.PacketId);
	if (!PendingReceive)
	{
		UE_LOG(LogBluevoxNetwork, Warning, TEXT("Received chunk for unknown PacketId=%u"), PacketChunk.PacketId);
		return;
	}
	
	if (PendingReceive->ReceivedChunks[PacketChunk.Index])
	{
		UE_LOG(LogBluevoxNetwork, Warning, TEXT("Received duplicate chunk for PacketId=%u, Index=%d"), PacketChunk.PacketId, PacketChunk.Index);
		// Already received this chunk
		return;
	}

	PendingReceive->LastPacketTime = FPlatformTime::Seconds();
	
	FMemory::Memcpy(PendingReceive->Data.GetData() + PacketChunk.Index * ChunkSize, PacketChunk.Data.GetData(), PacketChunk.Data.Num());
	PendingReceive->ReceivedChunks[PacketChunk.Index] = true;
	PendingReceive->MissingChunks--;

	if (PendingReceive->MissingChunks <= 0)
	{
		UE_LOG(LogBluevoxNetwork, Log, TEXT("All chunks received for PacketId=%u"), PacketChunk.PacketId);
		
		// All chunks received, process the packet
		UNetworkPacket* Packet = NewObject<UNetworkPacket>();

		Packet->DecompressAndSerialize(PendingReceive->Data);
		Packet->OnReceive(GameManager);
		PendingReceives.Remove(PacketChunk.PacketId);
		
		if (PendingReceive->bLocalPacket)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, PacketChunk]()
			{
				// Workaround to ensure we don't modify the map while iterating over it
				HandleConfirmedPacket(PacketChunk.PacketId);
			});
		} else
		{
			// Only run if we're in a client/server environment
			if (GameManager->bServer)
			{
				Cl_ConfirmReceivedPacket(PacketChunk.PacketId);
			} else
			{
				Sv_ConfirmReceivedPacket(PacketChunk.PacketId);
			}
		}
	}
}

UPlayerNetwork::UPlayerNetwork()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

UPlayerNetwork* UPlayerNetwork::Init(AGameManager* InGameManager,
	AMainController* InLocalController, APlayerState* InLocalPlayer)
{
	GameManager = InGameManager;
	LocalController = InLocalController;
	LocalPlayerState = InLocalPlayer;
	bServer = GameManager->bServer;
	return this;
}

void UPlayerNetwork::Cl_ConfirmReceivedPacket_Implementation(const uint32 PacketId)
{
	HandleConfirmedPacket(PacketId);
}

void UPlayerNetwork::Sv_ConfirmReceivedPacket_Implementation(const uint32 PacketId)
{
	HandleConfirmedPacket(PacketId);
}

void UPlayerNetwork::BeginPlay()
{
	Super::BeginPlay();
}

void UPlayerNetwork::SendToServer(UNetworkPacket* Packet)
{
	TArray<uint8> SerializedData;
	Packet->Compress(SerializedData);

	UE_LOG(LogBluevoxNetwork, Log, TEXT("Sending packet to server: PacketId=%u, Size=%d, PacketType=%s"), PacketIdCounter, SerializedData.Num(), *Packet->GetClass()->GetName());

	auto PendingSend = FPendingSend{
		PacketIdCounter++,
		Packet->GetClass(),
		GameManager->bServer,
		MoveTemp(SerializedData),
	};
	PendingSend.LastOffset = PendingSend.Data.Num() / ChunkSize * ChunkSize;

	Sv_SendPacketHeader(PendingSend);
	
	PendingSends.Add(PendingSend.PacketId, MoveTemp(PendingSend));
}

void UPlayerNetwork::SendToClient(UNetworkPacket* Packet)
{
	TArray<uint8> SerializedData;
	Packet->Compress(SerializedData);

	auto PendingSend = FPendingSend{
		PacketIdCounter++,
		Packet->GetClass(),
		GameManager->bClient,
		MoveTemp(SerializedData),
	};
	PendingSend.LastOffset = PendingSend.Data.Num() / ChunkSize * ChunkSize;
	PendingSends.Add(PendingSend.PacketId, PendingSend);

	Cl_SendPacketHeader(PendingSend);

	UE_LOG(LogBluevoxNetwork, Log, TEXT("Sending packet to player: PacketId=%u, Size=%d, PacketType=%s"), 
		PacketIdCounter - 1, SerializedData.Num(), *Packet->GetClass()->GetName());
}

void UPlayerNetwork::HandleConfirmedPacket(const uint32 PacketId)
{
	UE_LOG(LogBluevoxNetwork, Log, TEXT("Packet confirmed to be received by counterpart: PacketId=%u"), PacketId);
	PendingSends.Remove(PacketId);
}

void UPlayerNetwork::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LastResetTime + 1.0 < FPlatformTime::Seconds())
	{
		SentThisSecond = 0;
		LastResetTime = FPlatformTime::Seconds();
	}

	if (SentThisSecond >= MaxSentBytesPerSecond)
	{
		return;
	}
	
	for (auto& PendingSend : PendingSends)
	{
		const auto Continue = ProcessPendingSend(PendingSend.Value);
		if (!Continue)
		{
			break;
		}
	}

	// DEV check for resend
}

