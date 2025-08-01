// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameTickable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGameTickable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BLUEVOX_API IGameTickable
{
	GENERATED_BODY()

public:
	void GameTick(float DeltaTime);
};
