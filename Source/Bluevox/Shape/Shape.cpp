// Fill out your copyright notice in the Description page of Project Settings.


#include "Shape.h"

#include "Bluevox/Game/GameRules.h"
#include "Bluevox/ShapeMaterial/ShapeMaterialRegistry.h"

FName UShape::GetNameId() const
{
	UE_LOG(LogTemp, Fatal, TEXT("UShape::GetNameId not implemented for %s"), *GetName());
	return NAME_None;
}

void UShape::Render(UE::Geometry::FDynamicMesh3& Mesh, const EFace Face,
	const FLocalPosition& Position, int32 Size, int32 MaterialId)
{
}

void UShape::InitializeAllowedMaterials(UMaterialRegistry* Registry)
{
}
