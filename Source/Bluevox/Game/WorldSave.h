// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Chunk/Generator/FlatWorldGenerator.h"
#include "UObject/Object.h"
#include "WorldSave.generated.h"

class AMainController;
struct FRegionFile;
struct FRegionPosition;
class URegion;
class UWorldGenerator;

/**
 * 
 */
UCLASS()
class BLUEVOX_API UWorldSave : public UObject
{
	GENERATED_BODY()

	static void CreateSaveFolder(const FString& InWorldName);

	virtual void Serialize(FArchive& Ar) override;

public:
	static bool HasWorldSave(const FString& InWorldName)
	{
		return FPaths::FileExists(GetFullSavePath(InWorldName));
	}
	
	static UWorldSave* LoadWorldSave(AGameManager* InGameManager, const FString& InWorldName);

	static UWorldSave* CreateOrLoadWorldSave(AGameManager* InGameManager, const FString& InWorldName, const TSubclassOf<UWorldGenerator>& WorldGeneratorClass);

	static FString GetWorldsDir()
	{
		return FPaths::ProjectSavedDir() / TEXT("Worlds");
	}

	static FString GetSaveDir(const FString& WorldName)
	{
		return GetWorldsDir() / WorldName;
	}

	static FString GetRegionsDir(const FString& WorldName) 
	{
		return GetSaveDir(WorldName) / "Regions";
	}
	
	static FString GetNetworksDir(const FString& WorldName) 
	{
		return GetSaveDir(WorldName) / "Networks";
	}

	static FString GetPlayersDir(const FString& WorldName) 
	{
		return GetSaveDir(WorldName) / "Players";
	}

	static FString GetFullSavePath(const FString& WorldName)
	{
		return GetSaveDir(WorldName) / "world.dat";
	}

	TSharedPtr<FRegionFile> GetRegionFromDisk(const FRegionPosition& RegionPosition) const;

	bool SavePlayer(AMainController* PlayerController) const;

	bool LoadPlayer(AMainController* PlayerController) const;

	// DEV load network

	UPROPERTY()
	FString WorldName;

	UPROPERTY()
	int32 SaveVersion = 1;

	UPROPERTY()
	TSubclassOf<UWorldGenerator> WorldGeneratorClass = UFlatWorldGenerator::StaticClass();
	
	UPROPERTY()
	UWorldGenerator* WorldGenerator;
	
	void Save();
};
