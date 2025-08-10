// Fill out your copyright notice in the Description page of Project Settings.


#include "ShapeRegistry.h"

#include "Shape.h"
#include "Shapes/VoidShape.h"

int32 UShapeRegistry::RegisterShape(const FName& ShapeName, const TSubclassOf<UObject>& ShapeClass)
{
	UShape* Shape = Cast<UShape>(NewObject<UObject>(this, ShapeClass));
	Shape->InitializeData();
	const auto Index = RegisteredShapes.Add(Shape);
	ShapeByName.Add(ShapeName, Index);
	return Index;
}

void UShapeRegistry::RegisterAll()
{
	// Always register the VoidShape first, so it has ID 0
	const auto VoidShapeClass = UVoidShape::StaticClass();
	RegisterShape(VoidShapeClass->GetDefaultObject<UShape>()->GetNameId(), VoidShapeClass);
	
	TArray<UClass*> ShapeClasses;
	GetDerivedClasses(UShape::StaticClass(), ShapeClasses, true);

	for (UClass* ShapeClass : ShapeClasses)
	{
		if (ShapeClass->HasAnyClassFlags(CLASS_Abstract) || ShapeClass == VoidShapeClass)
		{
			continue;
		}

		const UShape* CDO = ShapeClass->GetDefaultObject<UShape>();
		RegisterShape(CDO->GetNameId(), ShapeClass);
	}
}

uint16 UShapeRegistry::GetShapeIdByName(const FName& ShapeName) const
{
	const auto Index = ShapeByName.Find(ShapeName);
	if (Index && RegisteredShapes.IsValidIndex(*Index))
	{
		return *Index;
	}

	return INDEX_NONE;
}

const UShape* UShapeRegistry::GetShapeById(const int32 ShapeId) const
{
	if (RegisteredShapes.IsValidIndex(ShapeId))
	{
		return RegisteredShapes[ShapeId];
	}

	UE_LOG(LogTemp, Warning, TEXT("Shape %d not found in registry"), ShapeId);
	return nullptr;
}

const UShape* UShapeRegistry::GetShapeByName(const FName& ShapeName) const
{
	const auto Index = ShapeByName.Find(ShapeName);
	if (Index && RegisteredShapes.IsValidIndex(*Index))
	{
		return RegisteredShapes[*Index];
	}

	UE_LOG(LogTemp, Warning, TEXT("Shape %s not found in registry"), *ShapeName.ToString());
	return nullptr;
}

void UShapeRegistry::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	// DEV should serialize their name id, then new object when loading
	// DEV should also consider when their name id are not found/registered
}
