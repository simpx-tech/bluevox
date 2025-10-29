#include "RegionFile.h"

#include "LogChunk.h"
#include "Bluevox/Game/GameConstants.h"
#include "Data/ChunkData.h"
#include "Position/LocalChunkPosition.h"
#include "Position/RegionPosition.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/BufferArchive.h"

void FRegionFile::Th_SaveChunk(const FLocalChunkPosition& Position, UChunkData* ChunkData)
{
	UE_LOG(LogChunk, Verbose, TEXT("Saving chunk data for position %s in disk."), *Position.ToString());

	if (!IsValid(ChunkData))
	{
		UE_LOG(LogChunk, Error, TEXT("Chunk data is null for position %s."), *Position.ToString());
		return;
	}

	
	const uint32 Index = Position.X + Position.Y * GameConstants::Region::Size;

	FBufferArchive Uncompressed;
	Uncompressed << ChunkData->Columns;

	// Serialize instances
	int32 NumCollections = ChunkData->InstanceCollections.Num();
	Uncompressed << NumCollections;

	for (auto& Pair : ChunkData->InstanceCollections)
	{
		FInstanceCollection& Collection = Pair.Value;
		Uncompressed << Collection;
	}

	TArray<uint8> Compressed;
	FArchiveSaveCompressedProxy Compressor(Compressed, NAME_Zlib);
	Compressor << Uncompressed;
	Compressor.Close();

	if (Compressor.GetError())
	{
		UE_LOG(LogChunk, Error, TEXT("Compression failed for chunk %s."), *Position.ToString());
		return;
	}

	if (!Th_WriteSegment(Index, Compressed))
	{
		UE_LOG(LogChunk, Error, TEXT("Failed to write chunk %s."), *Position.ToString());
	}
}

bool FRegionFile::Th_LoadChunk(const FLocalChunkPosition& Position, TArray<FChunkColumn>& OutColumns,
                                TMap<FPrimaryAssetId, FInstanceCollection>& OutInstances)
{
	const uint32 Index = Position.X + Position.Y * GameConstants::Region::Size;

	TArray<uint8> Compressed;
	if (!Th_ReadSegment(Index, Compressed)) return false;
	if (Compressed.Num() == 0) { OutColumns.Reset(); OutInstances.Empty(); return false; }

	FArchiveLoadCompressedProxy Decompressor(Compressed, NAME_Zlib);
	if (Decompressor.GetError())
	{
		UE_LOG(LogChunk, Error, TEXT("Invalid compressed data for chunk %s."), *Position.ToString());
		return false;
	}

	FBufferArchive Uncompressed;
	Decompressor << Uncompressed;
	Decompressor.Close();

	if (Decompressor.GetError())
	{
		UE_LOG(LogChunk, Error, TEXT("Decompression failed for chunk %s."), *Position.ToString());
		return false;
	}

	FMemoryReader Reader(Uncompressed, true);
	Reader << OutColumns;

	// Load instances if available (for backwards compatibility with old saves)
	if (!Reader.AtEnd())
	{
		int32 NumCollections = 0;
		Reader << NumCollections;

		OutInstances.Empty();
		for (int32 i = 0; i < NumCollections; ++i)
		{
			FInstanceCollection Collection;
			Reader << Collection;
			OutInstances.Add(Collection.InstanceTypeId, Collection);
		}
	}
	else
	{
		OutInstances.Empty();
	}

	return !Reader.IsError();
}

TSharedPtr<FRegionFile> FRegionFile::NewFromDisk(const FString& WorldName,
                                                 const FRegionPosition& RegionPosition)
{
	const auto RegionFilePath = UWorldSave::GetRegionsDir(WorldName) / FString::Printf(TEXT("region_%d_%d.dat"), RegionPosition.X, RegionPosition.Y);

	if (!FPaths::FileExists(RegionFilePath))
	{
		const auto File = CreateOnDisk(
			RegionFilePath,
			GameConstants::Region::File::SegmentSizeBytes,
			GameConstants::Region::Size * GameConstants::Region::Size
		);

		if (!File)
		{
			UE_LOG(LogChunk, Error, TEXT("Failed to create region file at %s"), *RegionFilePath);
			return nullptr;
		}

		return StaticCastSharedPtr<FRegionFile>(File);
	}

	return StaticCastSharedPtr<FRegionFile>(LoadFromDisk(RegionFilePath));
}
