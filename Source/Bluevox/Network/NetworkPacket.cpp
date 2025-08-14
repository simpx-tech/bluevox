// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkPacket.h"

#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/BufferArchive.h"

void UNetworkPacket::Compress(TArray<uint8>& OutData)
{
	
	FBufferArchive ChunkArchive;
	Serialize(ChunkArchive);

	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(OutData, NAME_Zlib);
	Compressor << ChunkArchive;
	Compressor.Flush();

	UE_LOG(LogTemp, Warning, TEXT("UNetworkPacket::Compressed %d to %d bytes"),
		ChunkArchive.Num(), OutData.Num());
}

void UNetworkPacket::DecompressAndSerialize(const TArray<uint8>& InData)
{
	FBufferArchive ChunkArchive;
	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(InData, NAME_Zlib);
	Decompressor << ChunkArchive;
	Decompressor.Flush();
	FMemoryReader Reader(ChunkArchive);
	Serialize(Reader);

	UE_LOG(LogTemp, Warning, TEXT("UNetworkPacket::Decompressed %d bytes to %d"),
		InData.Num(), ChunkArchive.Num());
}

void UNetworkPacket::OnReceive(AGameManager* GameManager)
{
}

void UNetworkPacket::Serialize(FArchive& Ar)
{
}
