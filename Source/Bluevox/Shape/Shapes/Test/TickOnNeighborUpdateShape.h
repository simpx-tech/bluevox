// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Shape/Shape.h"
#include "TickOnNeighborUpdateShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UTickOnNeighborUpdateShape : public UShape
{
	GENERATED_BODY()

	virtual bool ShouldTickOnNeighborUpdate() const override
	{
		return true;
	}

	virtual FName GetNameId() const override
	{
		return GameConstants::Shapes::GShape_Test_TickOnNeighborUpdate;
	}

	virtual void GameTick(AGameManager* AGameManager, const FLocalPosition& Position, uint16 Height, UChunkData* WhereData, float DeltaTime) const override;
};
