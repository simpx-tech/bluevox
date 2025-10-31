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

	// Serialize entities (flatten TSparseArray)
	TArray<FEntityRecord> EntitiesArray;
	EntitiesArray.Reserve(ChunkData->Entities.Num());
	for (const FEntityRecord& Rec : ChunkData->Entities)
	{
		EntitiesArray.Add(Rec);
	}
	int32 NumEntities = EntitiesArray.Num();
	Uncompressed << NumEntities;
	for (FEntityRecord& Rec : EntitiesArray)
	{
		Uncompressed << Rec;
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
                                TArray<FEntityRecord>& OutEntities)
{
	const uint32 Index = Position.X + Position.Y * GameConstants::Region::Size;

	TArray<uint8> Compressed;
	if (!Th_ReadSegment(Index, Compressed)) return false;
	if (Compressed.Num() == 0) { OutColumns.Reset(); return false; }

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

	// Load entities
	OutEntities.Empty();
	if (!Reader.AtEnd())
	{
		int32 NumEntities = 0;
		Reader << NumEntities;
		OutEntities.SetNum(NumEntities);
		for (int32 i = 0; i < NumEntities; ++i)
		{
			Reader << OutEntities[i];
			OutEntities[i].ArrayIndex = i; // assign stable index matching position in array
		}
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
