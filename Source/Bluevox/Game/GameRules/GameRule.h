// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameRule.generated.h"

class AGameManager;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UGameRule : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnSetup(AGameManager* InGameManager);

	virtual void PostSetup(AGameManager* InGameManager);
};
