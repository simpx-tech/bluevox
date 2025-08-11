// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerNetwork.h"

#include "NetworkLogs.h"
#include "NetworkPacket.h"
#include "PacketChunk.h"
#include "ServerNetworkPacket.h"
#include "Bluevox/Game/GameManager.h"

void UPlayerNetwork::Sv_SendPacketHeader_Implementation(const FPacketHeader& PacketHeader)
{
	if (PacketHeader.PacketType->IsChildOf(UServerNetworkPacket::StaticClass()))
	{
		// Client to Server packet
		if (GameManager->bClientOnly)
		{
			UE_LOG(LogPlayerNetwork, Warning, TEXT("Client tried to send server packet to server: PacketId=%u, PacketType=%s"), PacketHeader.PacketId, *PacketHeader.PacketType->GetName());
			return;
		}
	}
	
	HandlePacketHeader(PacketHeader);
}

void UPlayerNetwork::Cl_SendPacketHeader_Implementation(const FPacketHeader& PacketHeader)
{
	HandlePacketHeader(PacketHeader);
}

void UPlayerNetwork::TryToRunPacket(const uint32 PacketId, UNetworkPacket* Packet)
{
	if (PacketId == ExpectedPacketId)
	{
		UE_LOG(LogPlayerNetwork, Verbose, TEXT("Executing packet: PacketId=%u, PacketType=%s"), PacketId, *Packet->GetClass()->GetName());
		Packet->OnReceive(GameManager);
		ExpectedPacketId++;

		while (!PendingExecution.IsEmpty() && PendingExecution.HeapTop().PacketId == ExpectedPacketId)
		{
			FPendingExecution Item;
			PendingExecution.HeapPop(Item); 

			UE_LOG(LogPlayerNetwork, Verbose, TEXT("Executing queued packet: PacketId=%u, PacketType=%s"), 
				Item.PacketId, *Item.Packet->GetClass()->GetName());
		
			Item.Packet->OnReceive(GameManager);
		
			ExpectedPacketId++; 
		}
	}
	else if (PacketId > ExpectedPacketId)
	{
		UE_LOG(LogPlayerNetwork, Verbose, TEXT("PacketId %u is not the expected %u, queuing for later execution."), PacketId, ExpectedPacketId);
		PendingExecution.HeapPush(FPendingExecution{ PacketId, Packet });
	} else
	{
		UE_LOG(LogPlayerNetwork, Warning, TEXT("Received packet with PacketId %u, but expected %u. Ignoring."), PacketId, ExpectedPacketId);
	}
}

void UPlayerNetwork::HandlePacketHeader(const FPacketHeader& PacketHeader)
{
	UE_LOG(LogPlayerNetwork, Verbose, TEXT("Received packet header: PacketId=%u, TotalSize=%d, PacketType=%s"),
		PacketHeader.PacketId, PacketHeader.TotalSize, PacketHeader.PacketType ? *PacketHeader.PacketType->GetName() : TEXT("None"));

	FPendingReceive PendingReceive(PacketHeader);
	PendingReceive.Data.SetNumUninitialized(PacketHeader.TotalSize);
	const int32 NumChunks = (PacketHeader.TotalSize + ChunkSize - 1) / ChunkSize;
	PendingReceive.MissingChunks = NumChunks;
	PendingReceive.ReceivedChunks.Init(false, NumChunks);
	PendingReceives.Add(PacketHeader.PacketId, MoveTemp(PendingReceive));
}

bool UPlayerNetwork::ProcessPendingSend(FPendingSend& PendingSend)
{
	if (PendingSend.bSentAll)
	{
		return true;
	}

	const int32 End = PendingSend.Data.Num();
	while (PendingSend.Offset < End) {
		const int32 SizeToCopy = FMath::Min(End - PendingSend.Offset, static_cast<int32>(ChunkSize));
	
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

		// if (PendingSend.Offset >= PendingSend.LastOffset)
		// {
		// 	PendingSend.bSentAll = true;
		// }

		PendingSend.Offset += SizeToCopy;
		
		if (SentThisSecond >= MaxSentBytesPerSecond)
		{
			UE_LOG(LogPlayerNetwork, Verbose, TEXT("Hit max send limit, stopping at Offset=%d for PacketId=%u"), PendingSend.Offset, PendingSend.PacketId);
			return false;
		}
	}
	
	PendingSend.bSentAll = true;

	return true;
}

bool UPlayerNetwork::ProcessPendingResend(FPendingResend& PendingResend)
{
	if (!PendingSends.Contains(PendingResend.PacketId))
	{
		return true;
	}
	
	const auto& PendingSend = PendingSends[PendingResend.PacketId];
	const int32 NumChunks = (PendingSend.Data.Num() + ChunkSize - 1) / ChunkSize;
	const int32 MaxChunkIndex = NumChunks - 1;
	
	for (int32 Index = PendingResend.LastSentIndex + 1; Index < PendingResend.Indexes.Num(); ++Index)
	{
		const auto ChunkIndex = PendingResend.Indexes[Index];
		
		if (ChunkIndex < 0 || ChunkIndex > MaxChunkIndex)
		{
			UE_LOG(LogPlayerNetwork, Warning, TEXT("[ProcessPendingResend] Invalid chunk index %d for PacketId=%u"), ChunkIndex, PendingResend.PacketId);
			continue;
		}

		FPacketChunk PacketChunk;
		PacketChunk.PacketId = PendingResend.PacketId;
		PacketChunk.Index = ChunkIndex;
		
		const int32 Offset = ChunkIndex * ChunkSize;
		const int32 SizeToCopy = FMath::Min(PendingSend.Data.Num() - Offset, static_cast<int32>(ChunkSize));
		PacketChunk.Data.SetNumUninitialized(SizeToCopy);
		FMemory::Memcpy(PacketChunk.Data.GetData(), PendingSend.Data.GetData() + Offset, SizeToCopy);

		SentThisSecond += SizeToCopy;
		
		if (bServer)
		{
			Cl_ReceiveClientChunk(PacketChunk);
		}
		else
		{
			Sv_ReceiveServerChunk(PacketChunk);
		}

		if (SentThisSecond >= MaxSentBytesPerSecond && Index < PendingResend.Indexes.Num() - 1)
		{
			UE_LOG(LogPlayerNetwork, Verbose, TEXT("Hit max send limit, stopping at ChunkIndex=%d for PacketId=%u"), ChunkIndex, PendingResend.PacketId);
			
			PendingResend.LastSentIndex = Index;
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
	UE_LOG(LogPlayerNetwork, Verbose, TEXT("Received packet chunk: PacketId=%u, DataSize=%d, Index=%d"),
		PacketChunk.PacketId, PacketChunk.Data.Num(), PacketChunk.Index);
	
	const auto PendingReceive = PendingReceives.Find(PacketChunk.PacketId);
	if (!PendingReceive)
	{
		UE_LOG(LogPlayerNetwork, Warning, TEXT("Received chunk for unknown PacketId=%u"), PacketChunk.PacketId);
		return;
	}

	if (PacketChunk.Index < 0 || PacketChunk.Index >= PendingReceive->ReceivedChunks.Num()) {
		UE_LOG(LogTemp, Warning, TEXT("Received chunk with invalid index %d for PacketId=%u"), PacketChunk.Index, PacketChunk.PacketId);
		return;
	}
	
	if (PendingReceive->ReceivedChunks[PacketChunk.Index])
	{
		UE_LOG(LogPlayerNetwork, Warning, TEXT("Received duplicate chunk for PacketId=%u, Index=%d"), PacketChunk.PacketId, PacketChunk.Index);
		// Already received this chunk
		return;
	}

	PendingReceive->LastPacketTimeSecs = FPlatformTime::Seconds();
	
	FMemory::Memcpy(PendingReceive->Data.GetData() + PacketChunk.Index * ChunkSize, PacketChunk.Data.GetData(), PacketChunk.Data.Num());
	PendingReceive->ReceivedChunks[PacketChunk.Index] = true;
	PendingReceive->MissingChunks--;

	if (PendingReceive->MissingChunks <= 0)
	{
		UE_LOG(LogPlayerNetwork, Verbose, TEXT("All chunks received for PacketId=%u"), PacketChunk.PacketId);
		
		// All chunks received, process the packet
		UNetworkPacket* Packet = NewObject<UNetworkPacket>(GetTransientPackage(), PendingReceive->PacketType);

		Packet->DecompressAndSerialize(PendingReceive->Data);
		TryToRunPacket(PacketChunk.PacketId, Packet);
		
		if (PendingReceive->bLocalPacket)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, PacketChunk]
			{
				// Workaround to ensure we don't modify the map while iterating over it
				HandleConfirmedPacket(PacketChunk.PacketId);
			});
		} else
		{
			if (GameManager->bServer)
			{
				Cl_ConfirmReceivedPacket(PacketChunk.PacketId);
			} else
			{
				Sv_ConfirmReceivedPacket(PacketChunk.PacketId);
			}
		}

		PendingReceives.Remove(PacketChunk.PacketId);
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

void UPlayerNetwork::Cl_AskForResend_Implementation(const uint32 PacketId, const TArray<int32>& Indexes)
{
	// TODO check if these indexes were not even sent, if is the case, do not add to pending resends, same for server
	PendingResends.Add(FPendingResend{
		PacketId,
		Indexes,
	});
}

void UPlayerNetwork::Sv_AskForResend_Implementation(const uint32 PacketId, const TArray<int32>& Indexes)
{
	PendingResends.Add(FPendingResend{
		PacketId,
		Indexes,
	});
}

void UPlayerNetwork::SendToServer(UNetworkPacket* Packet)
{
	TArray<uint8> SerializedData;
	Packet->Compress(SerializedData);

	UE_LOG(LogPlayerNetwork, Verbose, TEXT("Sending packet to server: PacketId=%u, Size=%d, PacketType=%s"), PacketIdCounter, SerializedData.Num(), *Packet->GetClass()->GetName());

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

	UE_LOG(LogPlayerNetwork, Verbose, TEXT("Sending packet to player: PacketId=%u, Size=%d, PacketType=%s"), 
		PacketIdCounter - 1, SerializedData.Num(), *Packet->GetClass()->GetName());
}

void UPlayerNetwork::HandleConfirmedPacket(const uint32 PacketId)
{
	UE_LOG(LogPlayerNetwork, Verbose, TEXT("Packet confirmed to be received by counterpart: PacketId=%u"), PacketId);
	PendingSends.Remove(PacketId);
}

void UPlayerNetwork::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FPlatformTime::Seconds() > LastResetTime + 1.0)
	{
		SentThisSecond = 0;
		LastResetTime = FPlatformTime::Seconds();
	}

	if (SentThisSecond >= MaxSentBytesPerSecond)
	{
		return;
	}

	int32 Idx = 0;
	for (auto& PendingResend : PendingResends)
	{
		const auto ProcessedEverything = ProcessPendingResend(PendingResend);
		if (!ProcessedEverything)
		{
			break;
		}
		Idx++;
	}
	if (Idx > 0)
	{
		PendingResends.RemoveAt(0, Idx);
	}
	
	for (auto& PendingSend : PendingSends)
	{
		const auto Continue = ProcessPendingSend(PendingSend.Value);
		if (!Continue)
		{
			break;
		}
	}

	for (auto& [PacketId, PendingReceive] : PendingReceives)
	{
		if (FPlatformTime::Seconds() > PendingReceive.LastPacketTimeSecs + ResendTimeoutSecs)
		{
			TArray<int32> MissingChunkIndexes;

			for (int32 Index = 0; Index < PendingReceive.ReceivedChunks.Num(); ++Index)
			{
				if (!PendingReceive.ReceivedChunks[Index])
				{
					MissingChunkIndexes.Add(Index);
				}
			}
			
			UE_LOG(LogPlayerNetwork, Warning, TEXT("Ask to resend %d chunks for PacketId=%u"), MissingChunkIndexes.Num(), PacketId);
			PendingReceive.LastPacketTimeSecs = FPlatformTime::Seconds();
			
			if (bServer)
			{
				Cl_AskForResend(PacketId, MissingChunkIndexes);
			}
			else
			{
				Sv_AskForResend(PacketId, MissingChunkIndexes);
			}
		}
	}
}

