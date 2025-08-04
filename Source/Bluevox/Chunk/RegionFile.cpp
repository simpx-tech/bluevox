#include "RegionFile.h"

#include "Bluevox/Game/GameRules.h"
#include "Data/ChunkData.h"
#include "Position/LocalChunkPosition.h"
#include "Position/RegionPosition.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/BufferArchive.h"

void FRegionFile::Th_SaveChunk(const FLocalChunkPosition& Position, UChunkData* ChunkData)
{
	const uint32 Index = Position.X + Position.Y * GameRules::Region::Size;

	FBufferArchive ChunkArchive;
	ChunkData->Serialize(ChunkArchive);

	TArray<uint8> CompressedData;
	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, NAME_Zlib);
	Compressor << ChunkArchive;
	Compressor.Flush();
	
	const auto Success = Th_WriteSegment(Index, CompressedData);
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to write chunk data for position %s in region file."), *Position.ToString());
	}
}

void FRegionFile::Th_LoadChunk(const FLocalChunkPosition& Position, UChunkData* OutChunkDat)
{
	const uint32 Index = Position.X + Position.Y * GameRules::Region::Size;

	TArray<uint8> Data;
	if (!Th_ReadSegment(Index, Data))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to read chunk data for position %s in region file."), *Position.ToString());
		return;
	}
	
	FBufferArchive ChunkArchive;
	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(Data, NAME_Zlib);
	Decompressor << ChunkArchive;
	Decompressor.Flush();
	
	FMemoryReader MemoryReader(ChunkArchive, true);
	OutChunkDat->Serialize(MemoryReader);
}

TSharedPtr<FRegionFile> FRegionFile::NewFromDisk(const FString& WorldName,
                                                 const FRegionPosition& RegionPosition)
{
	const auto RegionFilePath = UWorldSave::GetRegionsDir(WorldName) / FString::Printf(TEXT("region_%d_%d.dat"), RegionPosition.X, RegionPosition.Y);

	if (!FPaths::FileExists(RegionFilePath))
	{
		const auto File = CreateOnDisk(
			RegionFilePath,
			GameRules::Region::File::SegmentSizeBytes,
			GameRules::Region::Size * GameRules::Region::Size * GameRules::Region::Size
		);

		if (!File)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create region file at %s"), *RegionFilePath);
			return nullptr;
		}

		return StaticCastSharedPtr<FRegionFile>(File);
	}

	return StaticCastSharedPtr<FRegionFile>(LoadFromDisk(RegionFilePath));
}
