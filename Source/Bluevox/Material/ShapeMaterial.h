// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShapeMaterial.generated.h"

/**
 * 
 */
UCLASS()
class BLUEVOX_API UShapeMaterial : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FName GetNameId() const
	{
		UE_LOG(LogTemp, Fatal, TEXT("UShapeMaterial::GetNameId not implemented for %s"), *GetName());
		return NAME_None;
	}

	UPROPERTY(EditAnywhere)
	UTexture2D* Texture = nullptr;
};
