// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShapeRegistry.generated.h"

class UShape;
/**
 * 
 */
UCLASS()
class BLUEVOX_API UShapeRegistry : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<const UShape*> RegisteredShapes;

	UPROPERTY()
	TMap<FName, int32> ShapeByName;

	void RegisterShape(const FName& ShapeName, const TSubclassOf<UObject>& ShapeClass);
	
public:
	void RegisterAll();
};
