// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Material/ShapeMaterial.h"
#include "VoidMaterial.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UVoidMaterial : public UShapeMaterial
{
	GENERATED_BODY()

public:
	virtual FName GetNameId() const override;
};
