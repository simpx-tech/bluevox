// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameRule.h"
#include "PlayerSetupGameRule.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UPlayerSetupGameRule : public UGameRule
{
	GENERATED_BODY()

public:
	virtual void OnSetup(AGameManager* InGameManager) override;
};
