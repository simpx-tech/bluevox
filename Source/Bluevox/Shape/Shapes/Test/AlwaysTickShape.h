// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Shape/Shape.h"
#include "AlwaysTickShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UAlwaysTickShape : public UShape
{
	GENERATED_BODY()

	virtual FName GetNameId() const override
	{
		return GameConstants::Shapes::GShape_Test_AlwaysTick;
	}

	virtual void GameTick(AGameManager* AGameManager, const FLocalPosition& Position, uint16 Height, UChunkData* WhereData, float DeltaTime) const override;
};
