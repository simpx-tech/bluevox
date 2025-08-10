// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LayerShape.h"
#include "StoneLayerShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UStoneLayerShape : public ULayerShape
{
	GENERATED_BODY()

protected:
	virtual uint16 GetMaterialId() const override;

	virtual FName GetNameId() const override;
};
