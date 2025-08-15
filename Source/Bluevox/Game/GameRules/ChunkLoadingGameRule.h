// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameRule.h"
#include "ChunkLoadingGameRule.generated.h"

class AMainController;
/**
 * 
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class BLUEVOX_API UChunkLoadingGameRule : public UGameRule
{
	GENERATED_BODY()

	UPROPERTY()
	AGameManager* GameManager = nullptr;

public:
	virtual void OnSetup(AGameManager* InGameManager) override;

	UFUNCTION()
	void OnPlayerJoin(AMainController* PlayerController);

	UFUNCTION()
	void OnPlayerLeave(AMainController* PlayerController);
};
