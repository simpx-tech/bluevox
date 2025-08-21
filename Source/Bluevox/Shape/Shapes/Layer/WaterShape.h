// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LayerShape.h"
#include "Bluevox/Game/GameConstants.h"
#include "Bluevox/Shape/Shape.h"
#include "WaterShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UWaterShape : public ULayerShape
{
	GENERATED_BODY()

public:
	virtual FName GetNameId() const override
	{
		return GameConstants::Shapes::GShape_Water;
	}

	virtual EFaceVisibility GetVisibility(EFace Face) const override
	{
		return EFaceVisibility::Transparent;
	}

	virtual bool ShouldTickOnPlace() const override
	{
		return true;
	}
	
	virtual bool ShouldTickOnLoad() const override
	{
		return true;
	}

	virtual bool ShouldTickOnNeighborUpdate() const override
	{
		return true;
	}

	virtual uint16 GetMaterialId() const override
	{
		return 1;
	}

	virtual void GameTick(AGameManager* GameManager, const FLocalPosition& Position, uint16 Height, UChunkData* OriginChunkData, float DeltaTime) const override;
};
