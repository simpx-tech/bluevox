// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LayerShape.h"
#include "DirtLayerShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UDirtLayerShape : public ULayerShape
{
	GENERATED_BODY()

	virtual FName GetNameId() const override;

	virtual uint16 GetMaterialId() const override;
};
