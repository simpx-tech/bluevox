// Fill out your copyright notice in the Description page of Project Settings.


#include "FreshDevWorldGameRule.h"

#include "Bluevox/Game/GameManager.h"
#include "Bluevox/Game/WorldSave.h"

void UFreshDevWorldGameRule::OnSetup(AGameManager* GameManager)
{
	IFileManager::Get().DeleteDirectory(*UWorldSave::GetWorldsDir(), false, true);

	GameManager->WorldSave = UWorldSave::CreateOrLoadWorldSave(GameManager, "TestWorld", UFlatWorldGenerator::StaticClass());

	GameManager->WorldSave->WorldGenerator = WorldGenerator->Init(GameManager);
}
