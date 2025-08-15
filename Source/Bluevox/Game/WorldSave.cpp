// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldSave.h"

#include "MainController.h"
#include "Bluevox/Chunk/RegionFile.h"
#include "Bluevox/Chunk/Generator/WorldGenerator.h"
#include "GameFramework/PlayerState.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

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
	Ar << WorldName;
	Ar << SaveVersion;
	Ar << SpawnPosition;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (Ar.IsLoading())
		{
			Ar << WorldGeneratorClass;
			WorldGenerator = NewObject<UWorldGenerator>(this, WorldGeneratorClass);
			WorldGenerator->Serialize(Ar);
		} else if (Ar.IsSaving())
		{
			Ar << WorldGeneratorClass;
			WorldGenerator->Serialize(Ar);
		}
	}
}

UWorldSave* UWorldSave::LoadWorldSave(AGameManager* InGameManager, const FString& InWorldName)
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
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	WorldSave->Serialize(Ar);
	WorldSave->bLoadedFromDisk = true;
	WorldSave->WorldGenerator->Init(InGameManager);

	return WorldSave;
}

UWorldSave* UWorldSave::CreateOrLoadWorldSave(AGameManager* InGameManager, const FString& InWorldName,
	const TSubclassOf<UWorldGenerator>& WorldGeneratorClass)
{
	if (HasWorldSave(InWorldName))
	{
		return LoadWorldSave(InGameManager, InWorldName);
	}

	const auto WorldSave = NewObject<UWorldSave>();
	WorldSave->WorldName = InWorldName;
	WorldSave->SaveVersion = 1;
	WorldSave->WorldGenerator = NewObject<UWorldGenerator>(WorldSave, WorldGeneratorClass)->Init(InGameManager);
	WorldSave->Save();
	
	return WorldSave;
}

TSharedPtr<FRegionFile> UWorldSave::GetRegionFromDisk(const FRegionPosition& RegionPosition) const
{
	return FRegionFile::NewFromDisk(WorldName, RegionPosition);
}

bool UWorldSave::SavePlayer(AMainController* PlayerController) const
{
	if (!PlayerController)
	{
		checkf(false, TEXT("PlayerController is null"));
		return false;
	}
	
	const auto PlayerState = PlayerController->GetPlayerState<APlayerState>();

	const auto FilePath = GetPlayersDir(WorldName) / FString::Printf(TEXT("%s.dat"), *PlayerState->GetPlayerName());

	TArray<uint8> ByteArray;
	FMemoryWriter MemoryWriter(ByteArray, true);
	PlayerController->SerializeForWorldSave(MemoryWriter);

	if (!FFileHelper::SaveArrayToFile(MoveTemp(ByteArray), *FilePath))
	{
		checkf(false, TEXT("Failed to save player data for %s"), *PlayerState->GetPlayerName());
		return false;
	}

	return true;
}

bool UWorldSave::LoadPlayer(AMainController* PlayerController) const
{
	if (!PlayerController)
	{
		checkf(false, TEXT("PlayerController is null"));
		return false;
	}

	const auto PlayerState = PlayerController->GetPlayerState<APlayerState>();
	const auto FilePath = GetPlayersDir(WorldName) / FString::Printf(TEXT("%s.dat"), *PlayerState->GetPlayerName());

	if (!FPaths::FileExists(FilePath))
	{
		PlayerController->SavedGlobalPosition = SpawnPosition;
		SavePlayer(PlayerController);
	}
	
	TArray<uint8> ByteArray;
	if (!FFileHelper::LoadFileToArray(ByteArray, *FilePath))
	{
		checkf(false, TEXT("Failed to load player data for %s"), *PlayerState->GetPlayerName());
		return false;
	}

	FMemoryReader MemoryReader(ByteArray, true);
	PlayerController->Serialize(MemoryReader);
	return true;
}

void UWorldSave::Save()
{
	TArray<uint8> ByteArray;
	FMemoryWriter MemoryWriter(ByteArray, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	
	CreateSaveFolder(WorldName);
	
	Serialize(Ar);

	if (!FFileHelper::SaveArrayToFile(MoveTemp(ByteArray), *GetFullSavePath(WorldName)))
	{
		checkf(false, TEXT("Failed to save world"));
	}
}
