// Fill out your copyright notice in the Description page of Project Settings.


#include "ShapeRegistry.h"

#include "Shape.h"

int32 UShapeRegistry::RegisterShape(const FName& ShapeName, const TSubclassOf<UObject>& ShapeClass)
{
	const UShape* Shape = Cast<UShape>(NewObject<UObject>(this, ShapeClass, ShapeName));
	const auto Index = RegisteredShapes.Add(Shape);
	ShapeByName.Add(ShapeName, Index);
	return Index;
}

void UShapeRegistry::RegisterAll()
{
	TArray<UShape*> Shapes;
	for (TObjectIterator<UShape> It; It; ++It)
	{
		if (It->IsA<UShape>() && It->GetClass() != UShape::StaticClass())
		{
			Shapes.Add(Cast<UShape>(*It));
		}
	}

	for (const auto Shape : Shapes)
	{
		RegisterShape(Shape->GetNameId(), Shape->GetClass());
	}
}

uint8 UShapeRegistry::GetShapeIdByName(const FName& ShapeName) const
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

	return nullptr;
}

const UShape* UShapeRegistry::GetShapeByName(const FName& ShapeName) const
{
	const auto Index = ShapeByName.Find(ShapeName);
	if (Index && RegisteredShapes.IsValidIndex(*Index))
	{
		return RegisteredShapes[*Index];
	}

	return nullptr;
}

void UShapeRegistry::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
	// DEV should serialize their name id, then new object when loading
	// DEV should also consider when their name id are not found/registered
	Ar << RegisteredShapes;
}
