// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LayerShape.h"
#include "GrassLayerShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UGrassLayerShape : public ULayerShape
{
	GENERATED_BODY()

	virtual FName GetNameId() const override;

	virtual uint16 GetMaterialId() const override;
};
