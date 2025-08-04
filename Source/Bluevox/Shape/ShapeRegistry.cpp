// Fill out your copyright notice in the Description page of Project Settings.


#include "ShapeRegistry.h"

#include "Shape.h"

void UShapeRegistry::RegisterShape(const FName& ShapeName, const TSubclassOf<UObject>& ShapeClass)
{
	const UShape* Shape = Cast<UShape>(NewObject<UObject>(this, ShapeClass, ShapeName));
	const auto Index = RegisteredShapes.Add(Shape);
	ShapeByName.Add(ShapeName, Index);
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
