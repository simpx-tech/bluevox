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

	int32 RegisterShape(const FName& ShapeName, const TSubclassOf<UObject>& ShapeClass);
	
public:
	void RegisterAll();

	uint16 GetShapeIdByName(const FName& ShapeName) const;

	const UShape* GetShapeById(const int32 ShapeId) const;

	const UShape* GetShapeByName(const FName& ShapeName) const;

	virtual void Serialize(FArchive& Ar) override;
};
