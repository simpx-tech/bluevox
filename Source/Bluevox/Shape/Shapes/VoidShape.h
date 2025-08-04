// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bluevox/Shape/Shape.h"
#include "VoidShape.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UVoidShape : public UShape
{
	GENERATED_BODY()

public:
	virtual FName GetNameId() const override;

	virtual int32 GetMaterialCost() const override;
};
