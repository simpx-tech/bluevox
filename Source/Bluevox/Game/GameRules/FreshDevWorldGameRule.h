// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameRule.h"
#include "FreshDevWorldGameRule.generated.h"

class UWorldGenerator;

/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UFreshDevWorldGameRule : public UGameRule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Instanced, meta = (ShowOnlyInnerProperties))
	TObjectPtr<UWorldGenerator> WorldGenerator;
	
	virtual void OnSetup(AGameManager* GameManager) override;
};
