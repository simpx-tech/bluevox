// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldSave.h"

void UWorldSave::CreateSaveFolder(const FString& InWorldName)
{
	if (!IFileManager::Get().MakeDirectory(*GetRegionsDir(InWorldName), true))
	{
		checkf(false, TEXT("Failed to create regions folder"));
	}

	if (!IFileManager::Get().MakeDirectory(*GetPlayersDir(InWorldName), true))
	{
		checkf(false, TEXT("Failed to create players folder"));
	}

	if (!IFileManager::Get().MakeDirectory(*GetNetworksDir(InWorldName), true))
	{
		checkf(false, TEXT("Failed to create networks folder"));
	}
}

void UWorldSave::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	Ar << WorldName;
	Ar << SaveVersion;
	Ar << ChunkGeneratorClass;
}

UWorldSave* UWorldSave::LoadWorldSave(const FString& InWorldName)
{
	if (!HasWorldSave(InWorldName))
	{
		checkf(false, TEXT("Trying to load a world that does not exist: %s"), *InWorldName);
		return nullptr;
	}

	TArray<uint8> ByteArray;
	if (!FFileHelper::LoadFileToArray(ByteArray, *GetFullSavePath(InWorldName)))
	{
		checkf(false, TEXT("Failed to load world"));
		return nullptr;
	}

	const auto WorldSave = NewObject<UWorldSave>();
	FMemoryReader MemoryReader(ByteArray, true);
	WorldSave->Serialize(MemoryReader);

	return WorldSave;
}

FRegionFile UWorldSave::GetRegionFromDisk(const FRegionPosition& RegionPosition) const
{
	return FRegionFile::NewFromDisk(WorldName, RegionPosition);
}

void UWorldSave::Save()
{
	TArray<uint8> ByteArray;
	FMemoryWriter MemoryWriter(ByteArray, true);

	CreateSaveFolder(WorldName);
	
	Serialize(MemoryWriter);

	if (!FFileHelper::SaveArrayToFile(MoveTemp(ByteArray), *GetFullSavePath(WorldName)))
	{
		checkf(false, TEXT("Failed to save world"));
	}
}
